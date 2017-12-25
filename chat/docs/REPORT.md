# Messenger program using IPC (:Server, Client)
## System programming Assignment, Interprocess communication

Participants : 김영진 2016025241, 김정현 2014004448
Date of preparation : Dec.25.2017 18:09:00

Github repository : https://github.com/smilu97/system-hyu

### 목표

IPC의 Shared Memory와 Message Queue를 이용하여 broadcast chat과 personal chat을 구현한다. 이 프로그램은 각각의 process로 정의된 server와 여러개의 client들로 구성된다. 본 프로그램에서 사용되는 shared memory의 경우 messenger log를 기록하기 위해 사용된다. Message queue에 존재하는 PID와 input내용을 저장할 수 있다. Shared memory에 대한 구조체는 아래에 상세하게 기술되어 있다. 

Server의 동작은 다음과 같이 정의된다. 각 Process의 message queue를 확인하여 broadcast 또는 personal chat인지 확인한 후, 전자의 경우 shared memory에 저장한 후 전체 터미널에 메세지 내용을 출력한다. 후자의 경우 shared memory에 저장하지 않고 특정 client로 보내는 process의 PID에 해당하는 message queue로 메세지를 전송해준다. 

Client의 동작은 다음과 같이 정의된다. 각 process에는 하나의 message queue가 할당이 되며, 이 자료구조를 이용하여 broadcast 또는 personal chat을 실행한다. Client를 실행하면서 실행 환경을 전자 또는 후자로 선택할 수 있다. Client는 자신에게 온 메세지를 어떤 process로 부터 온 것인지 확인할 수 있다.

### Structures and functions
본 프로그램에서는 server process와 client process를 둘 다 관리하기 위한 common field data structure와 common usage function이 필요하다. 또한 각각의 server process와 client process를 관리하기 위한 private function들이 존재한다. 각각의 용도를 아래에서 자세하게 설명한다. 함수의 경우, 불필요한 설명을 생략하고 핵심적인 함수를 구체적으로 설명한다.

### Structure & Functions : Common

#### Structure : Message

    typedef struct Message {
        int type;
        pid_t from_pid;
        pid_t to_pid;
        char msg[MSG_SIZE];
    } Message;

하나의 Message structure 객체는 위와 같이 정의된다. Message type, 발신 process pid, 수신 process pid, 그리고 메세지 하나의 크기가 MSG_SIZE(1024byte)로 정의되어 있다.

#### Structure : QMessage

    typedef struct QMessage {
        long type;
        Message msg;
    } QMessage;

메세지를 전송하기 위한 Qmessage structure는 위와 같이 정의된다. 식별을 위한 Qmessage type과 위에서 정의된 Message structure로 정의되어 있다. 

#### Structure : MessageCont, RegResult, UserLink, UserLinkNode

Server process에서 관리하는 shared memory는 doubly linked list의 circular queue의 형태로 유지된다. 각각의 node와 전체 doubly linked list를 관리하기 위한 구조체는 다음과 같다.

    typedef struct MessageCont {
        int start_idx;
        int msg_num;
        Message msg[MAX_MSG];
    } MessageCont;

전체 저장할 수 있는 message의 갯수가 100개로 제한되어 있다. 100개가 넘어가면 자동으로 새로운 message로 덮어씌우게 된다.

    typedef struct RegResult {
        int s_qid;
        int c_qid;
    } RegResult;

Server ID와 Client ID를 함께 저장한다. 이는 structure Common내에서 server process와의 연결을 수립하거나 해제한 결과가 저장되는 공간이다.

    typedef struct UserLink {
        pid_t pid;
        pthread_t th;
        int s_qid;
        int c_qid;
        struct UserLink * next;
        struct UserLink * prev;
        struct UserLinkNode * node;
    } UserLink;

    typedef struct UserLinkNode {
        UserLink * user;
        struct UserLinkNode * next;
        struct UserLinkNode * prev;
    } UserLinkNode;

사용중인 process ID와 shared memory 내에서 doubly linked list로 저장된 message를 관리하기 위한 prev, next에 대한 포인터와 현재 사용중인 UserLink node에 대한 포인터로 정의된다. 연결된 user들의 정보를 가리키는 hash container이다. Server process의 힙 공간에 실제 정보가 저장되므로, client process가 접근하지 않는것을 권장한다.

#### Structure : Common

    typedef struct Common {
        pid_t server_pid;

        pthread_mutex_t reg_mutex;
        pid_t waiting;
        RegResult reg_result;

        UserLink * users[USER_POOL_SIZE];
        UserLinkNode * first_user;
        MessageCont cont;
    } Common;

Common이라는 구조체를 이용하여 shared memory 전체를 관리할 수 있다. Server process의 PID가 존재한다. Server process와의 연결을 수립하거나 해제하는 데에 있어서 획득해야 되는 mutex를 함께 선언한다. Server process와 연결을 수립하거나 혹은 해제하고 있는 client process의 PID를 waiting으로 정의한다. Server process와의 연결과 해제를 저장하기 위한 공간으로  reg_result를 정의한다. 연결된 client process의 정보가 저장되는 container을 users, first_user로 정의한다. 전체적인 메세지가 저장되는 공간을 container의 약자인 cont로 정의한다. 

요약하여, Common은 server process와 client process들이 공유하는 shared memory가 가지고 있는 구조체이다. `SHM_ID` 라는 매크로 선언으로 shared memory의 key값이 `common.h` 에 선언되어 있다. Server process는 이 SHM_ID를 키 값으로 가지는 shared memory 공간을 생성해서 자신이 머신에서 살아있음을 client process들이 알 수 있게 해준다.

#### Function : void init_common(Common *p_common);

    void init_common(Common * p_common)
    {
        init_MessageCont(&(p_common->cont));
        p_common->server_pid = getpid();
        p_common->waiting = 0;
        p_common->first_user = NULL;
        pthread_mutex_init(&(p_common->reg_mutex), NULL);
        for(int idx = 0; idx < USER_POOL_SIZE; ++idx) {
            p_common->users[idx] = NULL;
        }
    }

Server process를 실행할때 할당되는 shared memory를 초기화하는 함수다. 이는 init_Messagecont함수를 호출하여 Messagecont 구조체의 인덱스와 갯수를 0으로 초기화 한다. 또한 Common 구조체 모든 항목들에 대한 초기화를 실행한다.

#### Function : void push_MessageCont(MessageCont * p_cont, char * msg, pid_t from_pid, pid_t to_pid);

    void push_MessageCont(MessageCont * p_cont, char * msg, pid_t from_pid, pid_t to_pid)
    {
        int idx = (p_cont->start_idx + p_cont->msg_num) % MAX_MSG;
        if(p_cont->msg_num >= MAX_MSG) {
            ++(p_cont->start_idx);
            --(p_cont->msg_num);
        }
        strcpy_cnt(p_cont->msg[idx].msg, msg, MSG_SIZE);
        p_cont->msg[idx].from_pid = from_pid;
        p_cont->msg[idx].to_pid = to_pid;
        ++(p_cont->msg_num);
    }

Message container내에 새로운 메세지를 넣은 함수다. 이는 broadcast mode로 선택되었을 때 server process가 모든 client process의 message queue로 보내거나, client간에 personal mode로 선택되어 메세지를 보낼 경우에 사용된다. Container에 넣기 전에 circular queue의 조건을 만족시키기 위한 index와 객체의 갯수를 확인한 후에 넣는다.

### Functions : Server

#### Function : int main(int argc, char** argv, char** env);

    int main(int argc, char** argv, char** env)
    {
        int res;
        if((res = init_server()) != 0) 
        	return res;
        return 0;
    }

Server process는 쉽게 말해 중개하는 역할을 가진 process이다. Client process의 요청에 따라서 수동적으로 일련의 일들을 수행하는데, 따라서 이는 처음 server을 초기화 한 후, client process의 연결이 올 때까지 대기한다. 

#### Function : int init_server();

    int init_server()
    {
       
        shm_id = shmget(SHM_ID, sizeof(Common), IPC_CREAT | SHMGET_GRANT);
        if(shm_id == -1) {
            fprintf(stderr, "Failed to shmget\n");
            return -1;
        }
        printf("Created shared mem\n");

        p_common = (Common*)shmat(shm_id, NULL, 0);
        printf("Attached shared mem\n");
      
        last_qid = INIT_LAST_QID;

        init_common(p_common);
        printf("Initialized common area\n");

        if(listen() == -1) return -1;

        return 0;
    }

Server process의 기능을 수행하기 위한 초기화 함수다. 먼저 server process와 client process간의 정보 공유를 위한 shared memory의 공간을 요청한다. Shared memory의 공간을 할당한다. Client process들과 통신을 하기 위한 message queue에 사용할 시작 번호를 last_qid에 설정한다. 앞서 할당한 shared memory 공간을 초기화한 후 client process의 연결을 대기한다.

#### Function : void destroy_server();

    void destroy_server()
    {
        while(p_common->first_user != NULL)
            remove_user(p_common->first_user->user->pid);

        shmdt(p_common);
        shmctl(shm_id, IPC_RMID, NULL);

        printf("Server exit");
        exit(1);
    }

Server을 해제하기 위해 모든 client들과의 연결을 끊고 할당했던 shared memory를 해제한다. 우선 모든 user 정보를 지운 후, 할당한 shared memory를 주소에서 분리시키고 해제한다.

#### Function : void sigusr1_handler(int signo); void sigusr2_handler(int signo);

다음 두 함수는 연결 요청과 해제를 의미하도록 사용된다. 전자는 client process의 연결 요청을 처리하고, 후자는 연결 해제를 처리한다.

    void sigusr1_handler(int signo)
    {
        pid_t pid = p_common->waiting;
        printf("Request detected from (%d)\n", pid);

        create_user(pid);
    }

Client는 연결 요청을 보내기 전에 shared memory 영역에 있는 waiting 변수의 값을 자신의 pid로 설정한다. 이때 다른 프로세스들과의 충돌을 방지하기 위해 reg_mutex를 획득한다. Server는 연결 요청이 들어왔을 때, 이 waiting변수를 확인하고 어떤 client process가 연결 요청을 하였는지 식별할 수 있다. 

해당 process를 위한 user 객체를 create_user(pid)함수를 이용하여 만든다. 

    void sigusr2_handler(int signo)
    {
        pid_t pid = p_common->waiting;
        printf("Request detected to disconnect from (%d)\n", pid);

        remove_user(pid);
    }

전자 함수와 동일하게 요청을 보낸 client process를 식별하기 위해 waiting 변수를 읽는다. 해당 client process를 위한 user객체를 해제한다.

#### Function : void sigint_handler(int signo);

    void sigint_handler(int signo)
    {
        destroy_server();
    }

갑자기 종료되더라도 적당한 수순을 밟고 종료되도록 만드는 함수다.


#### Function : int listen();

    int listen()
    {
        if(signal(SIGINT,   sigint_handler) == SIG_ERR) return -1;

        if(signal(SIGUSR1, sigusr1_handler) == SIG_ERR) return -1;
        if(signal(SIGUSR2, sigusr2_handler) == SIG_ERR) return -1;

        while(1) {
            pause();
        }

        return 0;
    }

Server process가 client process의 요청을 받아들이기 위한 대기 모드로 진입하는 함수다. 위에서 언급한 세 가지 함수가 사용된다. 대기하는 중, 갑작스러운 종료가 발생하여도 일정 수순을 밟은 후 종료될 수 있도록 만들어준다. 종료되지 않는 경우엔, client process의 연결의 요청 또는 해제에 대한 작업을 처리한다. listen 함수가 호출되었을 때, 각 signal의 핸들러를 변경해준다. 마지막으로, 부모 프로세스가 종료되지 않도록 무한히 대기하도록 만든다. 


#### Function : void * watch(void * varg);

User의 연결요청에 의해 그 user과의 통신을 위한 thread를 생성한다. 이는 create_user함수에서 생성된다. 그 thread는 그 user와의 통신을 만들어진 두 개의 message queue중에 server가 수신을 위해 만들어진 message queue에 무언가 들어있는지 계속적으로 검사한 후에, 상응하는 행동을 취한다. 아래 watch함수는 위의 행동을 하기 위해 pthread_create에 들어가는 함수다.

    void * watch(void * varg)
    {
        UserLink * usr = (UserLink*)varg;
        QMessage q_msg;
        Message * p_msg;
        int res;

        while(1) {

            res = msgrcv(usr->c_qid, &q_msg, sizeof(Message), MSG_TYPE, 0);
            if(res == -1) {
                printf("Error occured in %d\n", usr->pid);
                break;
            }
            p_msg = &(q_msg.msg);

            if(1) {
                printf("[%d, %s]: %s\n", usr->pid, p_msg->type == MT_BROAD ? "BROAD" : "PERSON", p_msg->msg);
            }

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

Message queue에 메세지가 들어가있는지 계속해서 확인한다. 메세지가 broadcast mode 또는 personal mode인지 확인을 해서 broadcast라면 메세지 저장소에 그것을 등록하고 모든 유저에게 새로운 매세지가 존재함을 알린다. 만약 personal 메세지라면, 이 메세지를 받는 user 객체를 찾은 뒤에 해당 process에 알려준다. 

#### Function : UserLink * find_user(pit_t pid);

    UserLink * find_user(pid_t pid)
    {
        int h_idx = hash_int((int)pid, USER_POOL_SIZE);
        
        UserLink * cur = p_common->users[h_idx];
        while(cur != NULL) {
            if(cur->pid == pid) return cur;
        }

        return NULL;
    }

연결되어 있는 user 객체들 중에 해당 pid를 가진 객체를 찾는다. 이러한 user 객체들은 pid를 키값으로 하는 해쉬맵에 포인터가 저장이 되어있다. 해쉬맵은 체인 해쉬맵으로 구현되어 있다. pid를 해쉬함수에 넣고 맵의 어느 리스트에 들어있는지 확인 한 후 pid를 가진 객체를 찾는다.


#### Function : UserLink * create_user(pid_t pid);	

    UserLink * create_user(pid_t pid)
    {
        UserLink * usr = (UserLink*)malloc(sizeof(UserLink));
        UserLinkNode * node = (UserLinkNode*)malloc(sizeof(UserLinkNode));
        usr->pid = pid;

        int h_idx = hash_int((int)pid, USER_POOL_SIZE);

        usr->next = p_common->users[h_idx];
        usr->prev = NULL;
        if(usr->next) usr->next->prev = usr;
        p_common->users[h_idx] = usr;

        node->user = usr;
        node->next = p_common->first_user;
        node->prev = NULL;
        p_common->first_user = node;
        usr->node = node;

        usr->s_qid = create_msg_queue();
        usr->c_qid = create_msg_queue();

        pthread_create(&(usr->th), NULL, watch, usr);

        p_common->reg_result.s_qid = usr->s_qid;
        p_common->reg_result.c_qid = usr->c_qid;

        kill(pid, SIGUSR1);

        return usr;
    }

새로운 유저와의 연결을 생성하는 함수다. 해쉬맵의 해당 리스트 맨 앞에 새로 추가된 객체를 추가한다. 그 후 UserLinkNode의 linked list에 넣는다. 

해당 유저의 프로세스와 통신하기 위해 새로운 메세지 큐를 2개 생성한다. s_qid를 번호로 가지는 메시지 큐는 서버가 메시지를 보내는 용도로, c_qid를 번호로 가지는 메시지 큐는 클라이언트가 메시지를 보내는 용도로 사용된다. 그렇기 때문에 서버는 c_qid 메시지큐를 계속 검사하며 클라이언트는 s_qid 메시지큐를 계속 검사한다.

유저의 연결요청에 대한 처리가 끝나면, 통신에 사용될 메세지 큐의 번호를 클라이언트에게 알려주기 위해 공유 메모리에 그 번호들을 적는다. 클라이언트들은 연결 요청에 대한 응답을 받으면 이 번호를 읽어서 통신을 시작한다. 

#### Function : int remove_user(pid_t pid);

    int remove_user(pid_t pid)
    {
        int h_idx = hash_int((int)pid, USER_POOL_SIZE);

        UserLink * cur = p_common->users[h_idx];
        while(cur != NULL) {
            if(cur->pid == pid) {
               
                pthread_cancel(cur->th);

                if(cur->next) cur->next->prev = cur->prev;
                if(cur->prev) cur->prev->next = cur->next;
                if(p_common->users[h_idx] == cur) p_common->users[h_idx] = cur->next;

                UserLinkNode * node = cur->node;
                if(node->next) node->next->prev = node->prev;
                if(node->prev) node->prev->next = node->next;
                if(p_common->first_user == node) p_common->first_user = node->next;

                msgctl(cur->s_qid, IPC_RMID, NULL);
                msgctl(cur->c_qid, IPC_RMID, NULL);

                free(node);
                free(cur);

                kill(pid, SIGUSR2);
                
                return 0;
            }
            cur = cur->next;
        }

        return -1;
    }

해당 유저와의 연결을 해제하는 함수이다. 해당 유저를 찾기 위해 유저객체가 체인 해쉬맵의 어느 리스트에 있는지 확인한다. 순차적으로 다음과 같은 순서로 진행한다. 해쉬맵에 존재하는 유저객체를 제외한다. 리스트에 존재하는 유저 객체를 제외한다. 유저와의 통신에 사용하던 두 개의 메시지큐를 삭제한다. 유저객체를 해제한다.

#### Function : int send_broadcast_msg(char * msg, pid_t pid);

    int send_broadcast_msg(char * msg, pid_t pid)
    {
        push_MessageCont(&(p_common->cont), msg, pid, 0);

        QMessage qmsg;
        qmsg.type = MSG_TYPE;
        qmsg.msg.type = MT_BROAD;
        qmsg.msg.from_pid = pid;
        qmsg.msg.to_pid = 0;
        strcpy_cnt(qmsg.msg.msg, msg, MSG_SIZE);

        UserLinkNode * cur = p_common->first_user;
        while(cur != NULL) {
            UserLink * user = cur->user;
            msgsnd(user->s_qid, &qmsg, sizeof(Message), 0);
            cur = cur->next;
        }

        return 0;
    }

공유 메모리의 메세지 컨테이너에 새로운 메세지를 넣고 연결되어 있는 모든 유저들에게 새로운 전체 메세지가 있음을 알린다. 메세지 컨테이너에 새로운 메세지를 넣는다. 메세지를 큐로 보내기 위한 객체를 준비한 후 모든 유저들에게 알린다.

#### Function : int send_personal_msg(UserLink * usr, pid_t from, char * msg);

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

특정 유저에게 메세지를 전송한다.

### Functions : Client

#### Function : int main(int argc, char** argv, char** env);

    int main(int argc, char** argv, char** env)
    {

        setvbuf(stdin, NULL, _IONBF, 0);
        setvbuf(stdout, NULL, _IONBF, 0);

        init_MessageCont(&personMsg);
        now_person = 0;

        connect();

        if(connected == 0) return -1;

        signal(SIGWINCH, update_winsize);
        update_winsize(0);
        
        char cmd;
        mode = MODE_INPUT;
        pid_t pid;

        pthread_create(&receiver, NULL, receive, NULL);

        printf("Input > ");
        while(scanf("%c", &cmd) != EOF) {
            switch(cmd) {
            case 'i':
                show_informations();
                break;
            case 'b':
                mode = MODE_BROAD;
                set_broadcast();
                break;
            case 'p':
                scanf("%d", &pid);
                mode = MODE_PERSON;
                set_personal(pid);
                break;
            case 'q':
                disconnect();
                exit(1);
                break;
            default:
                show_informations();
                break;
            }
            if(cmd != '\n' && cmd != 'b') while(getchar() != '\n');
            mode = MODE_INPUT;
            printf("Input > ");
        }
        pthread_cancel(receiver);

        return 0;
    }

개인 메세지를 보관하기 위한 메세지 컨테이너를 초기화한 후 서버에 연결을 요청한다. winsize를 사용할 수 있도록 만들어준다. SIGWINCH 시그널의 핸들러를 설정해서 지속적으로 winsize를 업데이트하도록 한다. 서버로부터 오는 메시지를 받기 위한 쓰레드를 생성한다. 

Client을 사용할 mode를 i, b, p, q 중에 선택한다.

#### Function : void connect();

    void connect()
    {

        int shm = shmget(SHM_ID, sizeof(Common), SHMGET_GRANT);
        if(shm == -1) {

            fprintf(stderr, "Failed to shmget. Maybe server is not opened\n");
            return;
        }
        printf("Found shared mem\n");

        p_common = (Common*)shmat(shm, NULL, 0);
        printf("Attached shared mem\n");

        my_pid = getpid();
        printf("your pid: %d\n", my_pid);

        pthread_mutex_lock(&(p_common->reg_mutex));

        p_common->waiting = my_pid;

        if(signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
            printf("Failed to set SIGUSR1 handler\n");
            return;
        }

        s_qid = -1;
        c_qid = -1;

        kill(p_common->server_pid, SIGUSR1);
        printf("Requested connection to server\n");

        pause();

        while(s_qid == -1);
        printf("Responsed OK to connect\n");
        
        connected = 1;

        if(signal(SIGINT, sigint_handler) == SIG_ERR) {
            printf("Failed to set SIGINT handler\n");
        }
    }

서버가 생성한 공유 메모리를 가져온다. 서버와 연결작업을 수행하기 위해 공유메모리 일부를 사용하는데, 다른 클라이언트들과의 충돌을 방지하기 위해 뮤텍스를 획득한다. 이 뮤텍스테 대한 해제는 이 함수에서는 이루어지지 않는다. 서버에서 연결에 대한 연결에 대한 처리가 모두 끝났을 때 보내는 SIGUSR1 시그널에 대한 핸들러에서 이루어진다.

연결을 요청하는 프로세스가 자신임을 알리기 위해 waiting 변수에 자신의 pid를 넣는다. 서버에서 연결 요청을 모두 처리 했을 때 보낼 시그널에 대한 핸들러를 설정한다. 서버에 연결 요청을 보낸다.

#### Function : void disconnect();

    void disconnect()
    {
        if(connected == 0) return;

        if(signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
            printf("Failed to set SIGUSR2 handler\n");
            return;
        }

        pthread_mutex_lock(&(p_common->reg_mutex));
        p_common->waiting = my_pid;
        kill(p_common->server_pid, SIGUSR2);  
        printf("Requested disconnection to server\n");
        pause();
        while(s_qid != -1);  
        printf("Responsed OK to disconnect\n");
        
        connected = 0;
    }

서버와의 연결을 해제하는 함수이다. 서버와의 연결해제 요청에 대한 응답 시그널(SIGUSR2)에 대한 핸들러를 설정한다. 

#### Function : void print_bmessages(), void print_pmessages();

    void print_bmessages()
    {
        clear_console();
        for(int i = 0; i < p_common->cont.msg_num; ++i) {
            int idx = (p_common->cont.start_idx + i) % MAX_MSG;
            Message * msg = p_common->cont.msg + idx;
            gotoxy(i+1, 0);
            printf("[%d]: %s\n", msg->from_pid, msg->msg);
        }
        gotoxy(winsize.ws_row-1, 0);
        input_buf[input_buf_len] = '\0';
        printf("Chat > %s", input_buf);
    }

    void print_pmessages()
    {
        clear_console();
        int line_idx = 0;
        for(int i = 0; i < personMsg.msg_num; ++i) {
            Message * msg = personMsg.msg + i;
            if(msg->from_pid == now_person || (msg->from_pid == my_pid && msg->to_pid == now_person)) {
                gotoxy((line_idx++) + 1, 0);
                printf("[%d]: %s\n", msg->from_pid, msg->msg);
            }
        }
        gotoxy(winsize.ws_row-1, 0);
        input_buf[input_buf_len] = '\0';
        printf("Chat > %s", input_buf);
    }

Broadcast mode와 personal mode로 설정된 client process가 메세지를 콘솔 화면에 출력하는 함수이다. 각 함수 모두, 해당 메세지를 가지고 있는 자료구조의 인덱스를 통해 접근한다. 이후에, 저장된 자료구조를 출력한다. 아래 gotoxy라는 함수를 이용하여, 콘솔화면 상, 사용자의 입력이 화면 하단에 보이도록 조정한다.



