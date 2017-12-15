
/*
 * This header file contains common rules or functions to server and client both
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>

#define CLEAR_CONSOLE "\033[H\033[J"

#define SHM_ID 4444
#define MSG_SIZE 1024
#define MAX_MSG 100
#define USER_POOL_SIZE 4096
#define MAX_USER 100

#define SHMGET_GRANT 0666
#define MSGGET_GRANT 0666

#define MSG_TYPE 1

#define MT_BROAD  1
#define MT_PERSON 2

/*
 * One message unit
 */
typedef struct Message {
    int type;
    pid_t from_pid;
    pid_t to_pid;
    char msg[MSG_SIZE];
} Message;

/*
 * Message including type for message queueing
 */
typedef struct QMessage {
    long type;
    Message msg;
} QMessage;

/*
 * Defined structure for a shared memory to contain Messages
 * Circular queue
 */
typedef struct MessageCont {
    int start_idx;
    int msg_num;
    Message msg[MAX_MSG];
} MessageCont;

typedef struct RegResult {
    int s_qid;
    int c_qid;
} RegResult;

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

typedef struct Common {
    pid_t server_pid;

    pthread_mutex_t reg_mutex;
    pid_t waiting;
    RegResult reg_result;

    UserLink * users[USER_POOL_SIZE];
    UserLinkNode * first_user;
    MessageCont cont;
} Common;

void gotoxy(int x, int y);
void init_common(Common * p_common);
void init_MessageCont(MessageCont * p_cont);
void push_MessageCont(MessageCont * p_cont, char * msg, pid_t from_pid, pid_t to_pid);
void strcpy_cnt(char * dest, char * src, int max_len);
int hash_int(int val, int mod);
void clear_console();
void clear_line();

#endif
