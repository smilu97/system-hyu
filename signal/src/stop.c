
/* Author: smilu97
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

void sig_fn()
{
    printf("Ctrl-C is pressed. Try Again\n");
}

int main(int argc, char** argv)
{
    // Prepare sigaction struct
    struct sigaction handler_sig_fn;
    memset(&handler_sig_fn, 0x00, sizeof(struct sigaction));
    handler_sig_fn.sa_handler = &sig_fn;

    // Alter SIGINT signal handler to sig_fn
    if(sigaction(SIGINT, &handler_sig_fn, NULL) < 0) {
        fprintf(stderr, "Failed to set SIGINT handler\n");
        return -1;
    }

    // Count sheeps
    unsigned long long sheep_count = 1;
    for(;;) {
        printf("%llu sheep...\n", sheep_count++);
        sleep(1);
    }

    return 0;
}
