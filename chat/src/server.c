
#include "server.h"

Common * p_common;
int last_qid;
int shm_id;

int main(int argc, char** argv, char** env)
{
    int res;

    if((res = init_server()) != 0) return res;

    return 0;
}

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

void destroy_server()
{
    while(p_common->first_user != NULL)
        remove_user(p_common->first_user->user->pid);
    printf("Server exit");
    shmdt(p_common);
    shmctl(shm_id, IPC_RMID, NULL);

    exit(1);
}

void sigusr1_handler(int signo)
{
    pid_t pid = p_common->waiting;
    printf("Request detected from (%d)\n", pid);
    create_user(pid);
}

void sigusr2_handler(int signo)
{
    pid_t pid = p_common->waiting;
    printf("Request detected to disconnect from (%d)\n", pid);
    remove_user(pid);
}

void sigint_handler(int signo)
{
    destroy_server();
}

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

int create_msg_queue()
{
    int qid = msgget(++last_qid, IPC_CREAT | MSGGET_GRANT);
    if(qid == -1) return -1;
    return qid;
}

UserLink * find_user(pid_t pid)
{
    int h_idx = hash_int((int)pid, USER_POOL_SIZE);
    
    UserLink * cur = p_common->users[h_idx];
    while(cur != NULL) {
        if(cur->pid == pid) return cur;
    }

    return NULL;
}

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

    usr->s_qid = create_msg_queue();
    usr->c_qid = create_msg_queue();

    pthread_create(&(usr->th), NULL, watch, usr);

    p_common->reg_result.s_qid = usr->s_qid;
    p_common->reg_result.c_qid = usr->c_qid;

    node->user = usr;
    node->next = p_common->first_user;
    node->prev = NULL;
    p_common->first_user = node;
    usr->node = node;

    kill(pid, SIGUSR1);

    return usr;
}

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

            free(node);
            free(cur);

            kill(pid, SIGUSR2);
            
            return 0;
        }
        cur = cur->next;
    }

    return -1;
}

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
