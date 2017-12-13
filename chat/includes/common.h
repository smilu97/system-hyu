
/*
 * This header file contains common rules or functions to server and client both
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SHM_ID 4444
#define MSG_SIZE 1024

typedef struct {
   long data_type;
   char buf[MSG_SIZE];
} BroadcastMessage;

typedef struct {
    long data_type;
    pid_t to_pid;
    char buf[MSG_SIZE];
} PersonalMessage;

void gotoxy(int x, int y);

