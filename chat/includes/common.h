
/*
 * This header file contains common rules or functions to server and client both
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>

#define SHM_ID 4444
#define MSG_SIZE 1024
#define MAX_MSG 100

/*
 * One message unit
 */
typedef struct {
   pid_t pid;
   char msg[MSG_SIZE];
} Message;

/*
 * Message including type for message queueing
 */
typedef struct  {
    long type;
    Message msg;
} QMessage;

/*
 * Defined structure for a shared memory to contain Messages
 * Circular queue
 */
typedef struct {
    pid_t server_pid;
    int start_idx;
    int msg_num;
    Message msg[MAX_MSG];
} MessageCont;

typedef struct {
    pid_t server_pid;
    MessageCont cont;
} Common;

void gotoxy(int x, int y);
void init_common(Common * p_common);
void init_MessageCont(MessageCont * p_cont);
void push_MessageCont(MessageCont * p_cont, char * msg);
void strcpy_cnt(char * dest, char * src, int max_len);

#endif
