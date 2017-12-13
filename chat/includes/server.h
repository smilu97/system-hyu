

#ifndef __SERVER_H__
#define __SERVER_H__

#include "common.h"

typedef struct  {
    int qid;
} WatcherArgs;

int init_server();
int destroy_server();
void sigint_handler(int signo);
void listen();
void * watch(void * varg);

#endif
