
#include "server.h"

Common * p_common;
int stop_server;

int main(int argc, char** argv, char** env)
{
    int res;

    if((res = init_server()) != 0) return res;
    if((res = destroy_server()) != 0) return res;

    return 0;
}

int init_server()
{
    int shm = shmget(SHM_ID, sizeof(Common), IPC_CREAT | IPC_EXCL | 0666);
    if(shm == -1) {
        fprintf(stderr, "Failed to shmget\n");
        return -1;
    }
    p_common = (Common*)shmat(shm, NULL, 0);
    init_common(p_common);

    stop_server = 0;
    if(signal(SIGINT, sigint_handler) == SIG_ERR) return -1;

    listen();

    return 0;
}

int destroy_server()
{
    if(shmdt(p_common) != 0) return -1;

    return 0;
}

void sigint_handler(int signo)
{
    stop_server = 1;
}

void listen()
{
    
}
