
/* Author: smilu97
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

typedef unsigned long long llu;
typedef llu target_type;

#define O_FILE_OPEN (O_RDWR)

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

target_type target_dest;
char target_filename[1024];
int fd;

void myerror(const char * str)
{
    fprintf(stderr, ANSI_COLOR_RED "%s\n" ANSI_COLOR_RESET, str);
}

void work();
void stop_count();

void deep_sleep();
int start_counting();
target_type read_target();
target_type my_atoi(const char * str);
void write_target(target_type val);

pid_t next_pid, root_pid;
clock_t begin_time;

pid_t pid[3];
int pidx;

target_type latest_target;

int main(int argc, char** argv)
{
    if(argc < 3) {
        printf("syntax: %s <target count> <target file>\n", argv[0]);
        exit(1);
    }

    target_dest = my_atoi(argv[1]);
    if(target_dest == 0) {
        printf("Invalid target count. Target count must be integer over 0\n");
        exit(1);
    }

    strcpy(target_filename, argv[2]);

    struct sigaction handler_work;
    struct sigaction handler_stop_count;

    memset(&handler_work, 0x00, sizeof(handler_work));
    memset(&handler_stop_count, 0x00, sizeof(handler_stop_count));

    handler_work.sa_handler = &work;
    handler_stop_count.sa_handler = &stop_count;

    if(sigaction(SIGUSR1, &handler_work, NULL) < 0) {
        myerror("Failed to set SIGUSR1 handler");
        return -1;
    }
    if(sigaction(SIGUSR2, &handler_stop_count, NULL) < 0) {
        myerror("Failed to set SIGUSR2 handler");
        return -1;
    }

    return start_counting();
}

int start_counting()
{
    if(access(target_filename, F_OK) != 0) {
        fd = open(target_filename, O_FILE_OPEN |O_CREAT, 0644);
        write_target(0);
        printf("Created new file\n");
    } else {
        fd = open(target_filename, O_FILE_OPEN);
        write_target(0);
        printf("Opened existing file\n");
    }

    // Get root process ID
    root_pid = pid[0] = getpid();

    // Get process ID of child 1
    if((pid[1] = fork()) == 0) {
        if((pid[2] = fork()) == 0) {
            next_pid = pid[0];
            begin_time = clock();
            kill(pid[0], SIGUSR1);
            deep_sleep();
        } else if(pid[2] < 0) {
            fprintf(stderr, "Failed to create new process3\n");
            kill(pid[0], SIGUSR2);
            exit(1);
        }
        next_pid = pid[2];
        deep_sleep();
    } else if(pid[1] < 0){
        fprintf(stderr, "Failed to create new process2\n");
        exit(1);
    }
    
    next_pid = pid[1];
    pidx = 0;
    deep_sleep();

    return 0;
}

void work()
{
    target_type t = read_target();

    if(t >= target_dest) {
        kill(root_pid, SIGUSR2);
        return;
    }

    write_target(++t);
    latest_target = t;

    // printf("(%d) Target: %llu -> %llu, send signal to %d\n", getpid(), t-1, t, next_pid);

    kill(next_pid, SIGUSR1);
}

void stop_count()
{
    if(next_pid == root_pid) { // child 2
        clock_t end_time = clock();
        printf("%f seconds passed\n", (double)(end_time - begin_time) / CLOCKS_PER_SEC);
    } else if(getpid() == root_pid) { // parent
        kill(pid[1], SIGUSR2);
        kill(pid[2], SIGUSR2);
    }
    exit(1);
}

target_type read_target()
{
    char buf[20];

    if(fd == 0) {
        fprintf(stderr, "Failed to read target\n");
    }
    pread(fd, buf, 20, 0);

    return my_atoi(buf);
}

void write_target(target_type val)
{
    char buf[20];
    int len;
    
    if(fd == 0) {
        fprintf(stderr, "Failed to write target\n");
    }

    ftruncate(fd, 0);
    sprintf(buf, "%llu\n", val);
    pwrite(fd, buf, strlen(buf) + 1, 0);
}

target_type my_atoi(const char * str)
{
    target_type ret = 0;
    for(const char * cur = str; (*cur) >= '0' && (*cur) <= '9'; ++cur) {
        ret = (ret * 10) + ((*cur) - '0');
    }
    return ret;
}

void deep_sleep()
{
    for(;;) sleep(10000);
}
