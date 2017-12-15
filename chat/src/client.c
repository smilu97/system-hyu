
#include "client.h"

/*
 * 서버에서 할당한 공유메모리를 부착한 주소
 */
Common * p_common;

/*
 * 개인 메시지를 저장하기 위한 컨테이너
 */
MessageCont personMsg;

/*
 * 지금 일대일 개인 대화하고 있는 프로세스의 pid
 */
pid_t now_person;

/*
 * 내 pid
 */
pid_t my_pid;

/*
 * 현재 터미널의 크기. 터미널의 크기가 바뀔 때 마다
 * SIGWINCH 시그널의 핸들러인 update_winsize함수가
 * 실행되어서 이 구조체를 업데이트한다.
 * 
 * 이 구조체를 읽어서 '거의' 항상 정확히 현재 터미널의 크기를 알 수 있다.
 */
struct winsize winsize;

/*
 * 서버에서 메시지를 받기 위한 메시지큐의 아이디
 */
int s_qid;

/*
 * 서버로 메시지를 보내기 위한 메시지큐의 아이디
 */
int c_qid;

/*
 * 현재 서버와 연결되어있는지
 */
int connected = 0;

/*
 * 현재 어떤 작업을 수행하고 있는지
 * 
 * 전체 채팅인지 개인 채팅인지, 이 둘 중에 고르고 있는지...
 */
int mode;

/*
 * 상대방에게 보내기 위해 쓰고 있는 메시지를 담을 버퍼
 * 
 * 화면이 업데이트될 때 마다 다시 써줘야 하기 때문에 한 글자 쓸 때 마다 저장한다.
 */
char input_buf[MSG_SIZE];
int input_buf_len = 0;

/*
 * 서버로 부터 오는 메시지를 받기 위해 따로 두는 쓰레드
 * 
 * (메인 쓰레드는 보내는 역할을 수행함)
 */
pthread_t receiver;

int main(int argc, char** argv, char** env)
{
    /*
     * 버퍼링을 강제로 해제
     */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /*
     * 개인 메시지를 담기 위한 컨테이너를 초기화
     */
    init_MessageCont(&personMsg);
    now_person = 0;

    /*
     * 서버와 연결
     */
    connect();

    if(connected == 0) return -1;

    /*
     * winsize를 사용할 수 있도록 만들어준다.
     * 
     * SIGWINCH 시그널의 핸들러를 설정해서 지속적으로 winsize를 업데이트하도록 한다.
     */
    signal(SIGWINCH, update_winsize);
    update_winsize(0);
    
    char cmd;
    mode = MODE_INPUT;
    pid_t pid;

    /*
     * 서버로부터 오는 메시지를 받기 위한 쓰레드를 생성한다.
     */
    pthread_create(&receiver, NULL, receive, NULL);

    /*
     * mode를 고르기 위한 창.
     */
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

/*
 * 서버와 연결한다.
 */
void connect()
{
    /*
     * 서버가 생성한 공유메모리를 가져온다.
     */
    int shm = shmget(SHM_ID, sizeof(Common), SHMGET_GRANT);
    if(shm == -1) {
        // 없으면 서버가 실행되지 않았을 가능성이 아주아주 높다 => 종료
        fprintf(stderr, "Failed to shmget. Maybe server is not opened\n");
        return;
    }
    printf("Found shared mem\n");

    /*
     * 공유메모리를 부착
     */
    p_common = (Common*)shmat(shm, NULL, 0);
    printf("Attached shared mem\n");

    /*
     * 내 pid를 식별
     */
    my_pid = getpid();
    printf("your pid: %d\n", my_pid);

    /*
     * 서버와 연결작업을 수행하기 위해서 공유메모리의 일부를 사용하는데,
     * 다른 클라이언트들과의 충돌을 방지하기 위해 뮤텍스 락을 얻는다.
     * 
     * 이 락에 대한 해제는 이 함수에서는 찾아볼 수가 없고, 서버에서
     * 연결에 대한 처리가 모두 끝났을 때 보내는 SIGUSR1 시그널에 대한
     * 핸들러에서 찾아볼 수 있다.
     */
    pthread_mutex_lock(&(p_common->reg_mutex));

    /*
     * 연결을 요청하는 프로세스가 자신임을 표시해놓는다.
     */
    p_common->waiting = my_pid;

    /*
     * 서버에서 연결요청을 모두 처리했을 떄 보낼 시그널에 대한
     * 핸들러를 설정한다.
     */
    if(signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
        printf("Failed to set SIGUSR1 handler\n");
        return;
    }

    s_qid = -1;
    c_qid = -1;

    /*
     * 서버에 연결 요청을 보낸다.
     */
    kill(p_common->server_pid, SIGUSR1);
    printf("Requested connection to server\n");

    pause();
    /*
     * 연결요청에 대한 응답 시그널이 오게되면 s_qid가 -1이 아닌
     * 메시지큐의 아이디로 설정이 된다. 그 때 까지 기다림
     */
    while(s_qid == -1);
    printf("Responsed OK to connect\n");
    
    connected = 1;

    /*
     * 강제로 이 프로세스가 종료되려 할 경우, 서버와의 연결을 해제하고
     * 종료될 수 있도록 한다.
     */
    if(signal(SIGINT, sigint_handler) == SIG_ERR) {
        printf("Failed to set SIGINT handler\n");
    }
}

/*
 * 서버와 연결을 해제
 */
void disconnect()
{
    if(connected == 0) return;

    /*
     * 서버와의 연결해제 요청에 대한 응답 시그널(SIGUSR2)에 대한 핸들러를 설정
     */
    if(signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
        printf("Failed to set SIGUSR2 handler\n");
        return;
    }
    
    /*
     * 연결해제 요청하는 프로세스가 자신임을 표시하기 위한 락
     */
    pthread_mutex_lock(&(p_common->reg_mutex));
    p_common->waiting = my_pid;
    kill(p_common->server_pid, SIGUSR2);  // 연결 해제 요청
    printf("Requested disconnection to server\n");
    pause();
    while(s_qid != -1);  // 연결 해제가 되어서 s_qid가 -1이 될 때까지 기다림
    printf("Responsed OK to disconnect\n");
    
    connected = 0;
}

/*
 * 연결 요청에 대한 응답 시그널이 왔을 때의 처리
 */
void sigusr1_handler(int signo)
{
    s_qid = p_common->reg_result.s_qid;
    c_qid = p_common->reg_result.c_qid;
    pthread_mutex_unlock(&(p_common->reg_mutex));
}

/*
 * 연결 해제에 대한 응답 시그널이 왔을 때의 처리
 */
void sigusr2_handler(int signo)
{
    s_qid = -1;
    c_qid = -1;
    pthread_mutex_unlock(&(p_common->reg_mutex));
}

void sigint_handler(int signo)
{
    if(connected) {
        disconnect();
        while(connected);
        exit(1);
    }
}

void update_winsize(int signo)
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
}

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

void show_informations()
{
    printf("'i': show informations\n");
    printf("'b': Broadcasted messages\n");
    printf("'p': Personal message (Argument needed: pid)\n");
    printf("'q': Quit\n");
}

void set_broadcast()
{
    getchar();
    input_buf_len = 0;
    print_bmessages();
    while(1) {
        char ch = getchar();
        if(ch == '\n') {
            input_buf[input_buf_len] = '\0';
            if(strcmp(input_buf, "q!") == 0) {
                break;
            }
            send_broadcast_msg(input_buf);
            input_buf_len = 0;
        } else {
            input_buf[input_buf_len++] = ch;
        }
    }
}

void set_personal(pid_t usr)
{
    if(now_person != usr) {
        init_MessageCont(&personMsg);
    }
    getchar();
    input_buf_len = 0;
    print_pmessages();
    while(1) {
        char ch = getchar();
        if(ch == '\n') {
            input_buf[input_buf_len] = '\0';
            if(strcmp(input_buf, "q!") == 0) {
                break;
            }
            send_personal_msg(input_buf, now_person);
            input_buf_len = 0;
        } else {
            input_buf[input_buf_len++] = ch;
        }
    }
}

void send_broadcast_msg(char * msg)
{
    QMessage qmsg;
    strcpy_cnt(qmsg.msg.msg, msg, MSG_SIZE);
    qmsg.msg.type = MT_BROAD;
    qmsg.type = MSG_TYPE;
    msgsnd(c_qid, &qmsg, sizeof(Message), 0);
}

void send_personal_msg(char * msg, pid_t to_pid)
{
    QMessage qmsg;
    strcpy_cnt(qmsg.msg.msg, msg, MSG_SIZE);
    qmsg.msg.type = MT_PERSON;
    qmsg.msg.from_pid = my_pid;
    qmsg.msg.to_pid = to_pid;
    qmsg.type = MSG_TYPE;
    msgsnd(c_qid, &qmsg, sizeof(Message), 0);
    
    push_MessageCont(&personMsg, msg, my_pid, to_pid);
    print_pmessages();
}

void * receive(void * varg)
{
    QMessage qmsg;

    while(1) {
        msgrcv(s_qid, &qmsg, sizeof(Message), MSG_TYPE, 0);
        if(mode == MODE_BROAD) {
            print_bmessages();
        } else if(mode == MODE_PERSON) {
            push_MessageCont(&personMsg, qmsg.msg.msg, qmsg.msg.from_pid, my_pid);
            print_pmessages();
        }
    }

    return NULL;
}
