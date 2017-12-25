
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
M

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
    pid_t server_pid;  // 서버의 pid

    /*
     * 서버와 연결을 수립하거나 해제할 때 얻어야 하는 락
     */
    pthread_mutex_t reg_mutex;
    pid_t waiting;  // 서버와 연결혹은 해제하고 있는 클라이언트의 pid
    RegResult reg_result;  // 서버와의 연결혹은 해제의 결과가 저장되는 곳

    /*
     * 연결된 유저들에 대한 정보를 가리키는 해쉬컨테이너
     * 서버의 힙 공간에 실제 정보가 들어있으므로 클라이언트에서는
     * 접근하지 않는 것을 권장함
     */
    UserLink * users[USER_POOL_SIZE];
    UserLinkNode * first_user;

    /*
     * 전체 메시지가 저장되는 곳
     */
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
