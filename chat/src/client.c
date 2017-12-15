
#include "client.h"

Common * p_common;

MessageCont personMsg;
pid_t now_person;

pid_t my_pid;
struct winsize winsize;
int s_qid;
int c_qid;
int connected = 0;
int mode;

char input_buf[MSG_SIZE];
int input_buf_len = 0;

pthread_t receiver;

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
    if(signal(SIGINT, sigint_handler) == SIG_ERR) {
        printf("Failed to set SIGINT handler\n");
    }
    s_qid = -1;
    c_qid = -1;
    kill(p_common->server_pid, SIGUSR1);
    printf("Requested connection to server\n");

    pause();
    while(s_qid == -1);
    printf("Responsed OK to connect\n");
    
    connected = 1;
}

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

void sigusr1_handler(int signo)
{
    s_qid = p_common->reg_result.s_qid;
    c_qid = p_common->reg_result.c_qid;
    pthread_mutex_unlock(&(p_common->reg_mutex));
}

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
