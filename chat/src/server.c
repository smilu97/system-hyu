
#include "server.h"

/*
 * 공유 메모리를 할당한 주소를 가리키기 위함
 */
Common * p_common;

/*
 * 마지막으로 할당된 메시지큐의 아이디
 */
int last_qid;

/*
 * 할당한 공유메모리의 아이디. 해제할 때 필요하기 때문에
 * 가지고있어야 한다.
 */
int shm_id;

int main(int argc, char** argv, char** env)
{
    int res;

    /*
     * 서버를 실행시킨다.
     */
    if((res = init_server()) != 0) return res;

    return 0;
}

/*
 * 서버의 기능을 수행하기 위한 모든 준비를 함
 */
int init_server()
{
    /*
     * 서버와 클라이언트들 간의 정보 공유를 위한 공유메모리 공간을 요청한다.
     */
    shm_id = shmget(SHM_ID, sizeof(Common), IPC_CREAT | SHMGET_GRANT);
    if(shm_id == -1) {
        fprintf(stderr, "Failed to shmget\n");
        return -1;
    }
    printf("Created shared mem\n");

    /*
     * 공유메모리 공간을 할당한다.
     */
    p_common = (Common*)shmat(shm_id, NULL, 0);
    printf("Attached shared mem\n");

    /*
     * 클라이언트들과 통신하기 위한 메시지 큐에 붙일 번호의 시작번호를 설정한다.
     */
    last_qid = INIT_LAST_QID;

    /*
     * 할당한 공유메모리 공간을 초기화한다.
     */
    init_common(p_common);
    printf("Initialized common area\n");

    /*
     * 클라이언트들의 연결을 계속 기다린다.
     */
    if(listen() == -1) return -1;

    return 0;
}

/*
 * 서버를 종료하기 위해 모든 클라이언트들과의 연결을 끊고 할당했던 공유메모리를 해제한다.
 */
void destroy_server()
{
    /*
     * 모든 유저정보를 지운다.
     */
    while(p_common->first_user != NULL)
        remove_user(p_common->first_user->user->pid);

    /*
     * 할당한 공유메모리를 주소에서 분리시키고 해제한다
     */
    shmdt(p_common);
    shmctl(shm_id, IPC_RMID, NULL);

    printf("Server exit");
    exit(1);  // 프로세스를 강제종료
}

/*
 * SIGUSR1은 연결요청을 의미하도록 사용된다
 * 클라이언트에서 들어온 연결요청을 처리한다
 */
void sigusr1_handler(int signo)
{
    /*
     * 클라이언트는 연결요청을 보내기 전에 공유메모리 영역에 있는 pid_t waiting
     * 변수의 값을 자신의 pid로 설정한다.
     * 
     * 물론 이때, 클라이언트는 다른 프로세스와의 충돌을 방지하기 위해 waiting변수를 설정하기 전에
     * 공유메모리의 reg_mutex의 락을 얻는다.
     * 
     * 서버는 연결요청이 들어왔을 때, 이 waiting변수를 보고 어떤 프로세스가 연결 요청을 보냈는지
     * 인식할 수 있게 된다.
     */
    pid_t pid = p_common->waiting;
    printf("Request detected from (%d)\n", pid);

    /*
     * 해당 프로세스를 위한 유저객체를 만든다
     * 
     * 유저객체에는 해당 프로세스와 통신하기 위한 두 개의 메시지큐의 번호와
     * 클라이언트 프로세스의 pid, 프로세스와 통신하기 위해 만들어진 쓰레드 객체
     * 등이 들어있다.
     */
    create_user(pid);
}

/*
 * SIGUSR2는 연결해제를 의미하도록 사용된다.
 * 클라이언트에서 들어온 연결해제 요청을 처리한다.
 */
void sigusr2_handler(int signo)
{
    /*
     * SIGUSR1의 경우와 마찬가지로 요청을 보낸 클라이언트가 누구인지 식별하기 위해
     * waiting변수를 읽는다.
     */
    pid_t pid = p_common->waiting;
    printf("Request detected to disconnect from (%d)\n", pid);

    /*
     * 해당 프로세스를 위한 유저객체를 해제한다
     */
    remove_user(pid);
}

/*
 * 갑자기 종료되더라도 적당한 수순을 밟고 종료되도록 한다
 */
void sigint_handler(int signo)
{
    destroy_server();
}

/*
 * 서버가 클라이언트의 요청들을 받아들이기 위한 대기모드로 들어간다
 */
int listen()
{
    /*
     * 갑자기 종료되더라고 적당한 수순을 밟고 종료될 수 있도록 핸들러를 변경해준다.
     */
    if(signal(SIGINT,   sigint_handler) == SIG_ERR) return -1;

    /*
     * SIGUSR1, SIGUSR2는 각각, 연결, 연결해제 요청을 의미하는 시그널로
     * 사용된다. listen함수가 호출되었을 때, 각 시그널의 핸들러를 변경해준다.
     */
    if(signal(SIGUSR1, sigusr1_handler) == SIG_ERR) return -1;
    if(signal(SIGUSR2, sigusr2_handler) == SIG_ERR) return -1;

    /*
     * 부모 프로세스가 종료되지 않도록 무한히 대기해준다.
     */
    while(1) {
        pause();
    }

    return 0;
}

/*
 * 유저가 연결요청을 하면, 그 유저와의 통신을 위한 쓰레드를 하나 생성한다.
 * (create_user 함수에서 만들어짐)
 * 그 쓰레드는 그 유저와의 통신을 위해 만들어진 두 개의 메시지 큐중에, 서버가
 * 받기만을 위해 만들어진 메시지 큐에 무언가 들어있는지 계속적으로 검사하고, 그에
 * 맞춘 행동을 취한다.
 * 
 * 아래 watch함수는 위의 행동을 하기 위해 pthread_create함수 안에 들어갈 함수이다.
 */
void * watch(void * varg)
{
    UserLink * usr = (UserLink*)varg;
    QMessage q_msg;
    Message * p_msg;
    int res;

    while(1) {
        /*
         * 메시지 큐에 메시지가 들어있는지 확인한다.
         */
        res = msgrcv(usr->c_qid, &q_msg, sizeof(Message), MSG_TYPE, 0);
        if(res == -1) {
            printf("Error occured in %d\n", usr->pid);
            break;
        }
        p_msg = &(q_msg.msg);

        if(1) {
            printf("[%d, %s]: %s\n", usr->pid, p_msg->type == MT_BROAD ? "BROAD" : "PERSON", p_msg->msg);
        }

        /*
         * 메시지가 전체 채팅을 위한 것인지, 개인 채팅을 위한 것인지 판별해서
         * 
         * 전체 채팅을 위한 것이라면 메시지 저장소에 그것을 등록하고 모든 유저에게
         * 새로운 전체 메시지가 있음을 알린다.
         * 
         * 개인 채팅을 위한 것이라면 이 메시지를 받는 유저의 유저객체를 찾은 뒤에
         * 해당 프로세스에게 알린다.
         */
        if(p_msg->type == MT_BROAD) {
            send_broadcast_msg(p_msg->msg, usr->pid);
        } else if(p_msg->type == MT_PERSON) {
            UserLink * target = find_user(p_msg->to_pid);
            if(target != NULL) {
                send_personal_msg(target, usr->pid, p_msg->msg);
            }
        }
    }

    return NULL;
}

/*
 * 메시지 큐를 하나 만든다.
 */
int create_msg_queue()
{
    int qid = msgget(++last_qid, IPC_CREAT | MSGGET_GRANT);
    if(qid == -1) return -1;
    return qid;
}

/*
 * 연결되어있는 유저들의 유저객체들 중에 해당 pid를 가진 유저의 유저객체를 찾는다
 */
UserLink * find_user(pid_t pid)
{
    /*
     * 유저객체들은 pid를 키값으로 하는 해쉬맵에 포인터가 저장이 되어있다.
     * 
     * 해쉬맵은 체인해쉬맵으로 구현되어있다.
     * 
     * pid를 해쉬함수에 넣고 맵의 어느 리스트에 들어있는지 확인한 후 pid를
     * 가진 유저객체를 찾는다.
     */
    int h_idx = hash_int((int)pid, USER_POOL_SIZE);
    
    UserLink * cur = p_common->users[h_idx];
    while(cur != NULL) {
        if(cur->pid == pid) return cur;
    }

    return NULL;
}

/*
 * 새로운 유저와의 연결을 생성함
 */
UserLink * create_user(pid_t pid)
{
    /*
     * 유저 객체를 위한 메모리 영역을 할당한다.
     * UserLink 가 유저객체의 정보를 담고 있으며,
     * UserLinkNode는 해쉬맵으로 관리되는 UserLink대신, Linear Iteration을 
     * 지원하기 위해 따로 관리되는 링크드 리스트 노드이다.
     */
    UserLink * usr = (UserLink*)malloc(sizeof(UserLink));
    UserLinkNode * node = (UserLinkNode*)malloc(sizeof(UserLinkNode));
    usr->pid = pid;

    /*
     * 해쉬 맵의 해당 리스트 맨 앞에 새로운 유저객체를 넣는다.
     */
    int h_idx = hash_int((int)pid, USER_POOL_SIZE);

    usr->next = p_common->users[h_idx];
    usr->prev = NULL;
    if(usr->next) usr->next->prev = usr;
    p_common->users[h_idx] = usr;

    /*
     * UserLinkNode의 링크드리스트에 넣는다.
     */
    node->user = usr;
    node->next = p_common->first_user;
    node->prev = NULL;
    p_common->first_user = node;
    usr->node = node;

    /*
     * 해당 유저의 프로세스와 통신하기 위해 새로운 메시지 큐를 2개 만든다.
     * s_qid를 번호로 가지는 메시지 큐는 서버가 메시지를 보내는 용도로,
     * c_qid를 번호로 가지는 메시지 큐는 클라이언트가 메시지를 보내는 용도로 사용된다.
     * 
     * 그렇기 때문에 서버는 c_qid 메시지큐를 계속 검사하며
     * 클라이언트는 s_qid 메시지큐를 계속 검사한다.
     */
    usr->s_qid = create_msg_queue();
    usr->c_qid = create_msg_queue();

    /*
     * 모든 유저들의 메시지 큐들을 개별적으로 검사하기 위해 유저마다 쓰레드를
     * 하나 씩 생성해준다.
     */
    pthread_create(&(usr->th), NULL, watch, usr);

    /*
     * 유저의 연결요청에 대한 처리가 끝나면, 통신에 사용될 메시지큐의 번호를
     * 클라이언트에게 알려주기 위해 공유메모리에 그 번호들을 적어놓는다.
     * 
     * 클라이언트들은 연결요청에 대한 응답을 받으면 이 번호를 읽어서 통신을
     * 시작한다.
     */
    p_common->reg_result.s_qid = usr->s_qid;
    p_common->reg_result.c_qid = usr->c_qid;

    /*
     * 클라이언트에게 연결 요청이 모두 처리되었음을 알려준다.
     */
    kill(pid, SIGUSR1);

    /*
     * 처리가 끝난 유저의 유저객체를 반환함.
     */
    return usr;
}

/*
 * 해당 유저와의 연결을 해제함.
 */
int remove_user(pid_t pid)
{
    /*
     * 해당 유저를 찾기 위해 유저객체가 체인 해쉬맵의 어느 리스트에 있는지 확인
     */
    int h_idx = hash_int((int)pid, USER_POOL_SIZE);

    UserLink * cur = p_common->users[h_idx];
    while(cur != NULL) {
        if(cur->pid == pid) {
            /*
             * 해당 프로세스의 메시지를 검사하고 있던 쓰레드를 강제종료
             */
            pthread_cancel(cur->th);

            /*
             * 유저객체를 해쉬맵에서 제외
             */
            if(cur->next) cur->next->prev = cur->prev;
            if(cur->prev) cur->prev->next = cur->next;
            if(p_common->users[h_idx] == cur) p_common->users[h_idx] = cur->next;

            /*
             * 유저객체를 리스트에서 제외
             */
            UserLinkNode * node = cur->node;
            if(node->next) node->next->prev = node->prev;
            if(node->prev) node->prev->next = node->next;
            if(p_common->first_user == node) p_common->first_user = node->next;

            /*
             * 유저와의 통신에 사용하던 두 개의 메시지큐를 삭제
             */
            msgctl(cur->s_qid, IPC_RMID, NULL);
            msgctl(cur->c_qid, IPC_RMID, NULL);

            /*
             * 유저객체를 해제
             */
            free(node);
            free(cur);

            /*
             * 연결 해제를 완료했음을 클라이언트에게 알림
             */
            kill(pid, SIGUSR2);
            
            return 0;
        }
        cur = cur->next;
    }

    return -1;
}

/*
 * 공유메모리의 메시지 컨테이너에 새로운 메시지를 넣고
 * 연결되어있는 모든 유저들에게 새로운 전체메시지가 있음을 알림
 */
int send_broadcast_msg(char * msg, pid_t pid)
{
    /*
     * 메시지 컨테이너에 새로운 메시지를 넣음
     */
    push_MessageCont(&(p_common->cont), msg, pid, 0);

    /*
     * 메시지를 큐로 보내기 위한 객체 준비
     */
    QMessage qmsg;
    qmsg.type = MSG_TYPE;
    qmsg.msg.type = MT_BROAD;
    qmsg.msg.from_pid = pid;
    qmsg.msg.to_pid = 0;
    strcpy_cnt(qmsg.msg.msg, msg, MSG_SIZE);

    /*
     * 모든 유저들에게 알림
     */
    UserLinkNode * cur = p_common->first_user;
    while(cur != NULL) {
        UserLink * user = cur->user;
        msgsnd(user->s_qid, &qmsg, sizeof(Message), 0);
        cur = cur->next;
    }

    return 0;
}

/*
 * 특정 유저에게 메시지를 보냄
 */
int send_personal_msg(UserLink * usr, pid_t from, char * msg)
{
    QMessage qmsg;
    qmsg.type = MSG_TYPE;
    qmsg.msg.type = MT_BROAD;
    qmsg.msg.from_pid = from;
    qmsg.msg.to_pid = usr->pid;
    strcpy_cnt(qmsg.msg.msg, msg, MSG_SIZE);

    msgsnd(usr->s_qid, &qmsg, sizeof(Message), 0);

    return 0;
}
