

#ifndef __SERVER_H__
#define __SERVER_H__

#include "common.h"

#define INIT_LAST_QID 999

int init_server();
void destroy_server();
void sigint_handler(int signo);
void sigusr1_handler(int signo);
int listen();
void * watch(void * varg);

int create_msg_queue();
UserLink * find_user(pid_t pid);
UserLink * create_user(pid_t pid);
int remove_user(pid_t pid);

int send_broadcast_msg(char * msg);
int send_personal_msg(UserLink * usr, pid_t from, char * msg);

#endif
