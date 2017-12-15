

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "common.h"

#define MODE_INPUT 0
#define MODE_BROAD 1
#define MODE_PERSON 2

void connect();
void disconnect();
void sigusr1_handler(int signo);
void sigusr2_handler(int signo);
void sigint_handler(int signo);

void update_winsize(int signo);

void print_bmessages();
void print_pmessages();

void show_informations();
void set_broadcast();
void set_personal();

void send_broadcast_msg(char * msg);
void send_personal_msg(char * msg, pid_t to_pid);

void * receive(void * varg);

#endif
