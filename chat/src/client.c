
#include "client.h"

Common * p_common;
pid_t my_pid;

int main(int argc, char** argv, char** env)
{
    connect();

    return 0;
}

void connect()
{
    int shm = shmget(SHM_ID, sizeof(Common), SHMGET_GRANT);
    if(shm == -1) {
        fprintf(stderr, "Failed to shmget. Maybe server is not opened\n");
        return -1;
    }
    printf("Found shared mem\n");
    p_common = (Common*)shmat(shm, NULL, 0);
    printf("Attached shared mem\n");

    my_pid = getpid();
    pthread_mutex_lock(&(p_common->reg_mutex));
    p_common->waiting = my_pid;
    if(signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
        printf("Failed to set SIGUSR1 handler\n");
        return;
    }
    kill(p_common->server_pid, SIGUSR1);
}

void sigusr1_handler(int signo)
{
    pthread_mutex_unlock(&(p_common->reg_mutex));
}