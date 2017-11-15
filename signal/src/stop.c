
/* Author: smilu97
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void sig_fn()
{
    printf("Ctrl-C is pressed. Try Again\n");
}

int main(int argc, char** argv)
{
    signal(SIGINT, sig_fn);

    unsigned long long sheep_count = 1;
    for(;;) {
        printf("%llu sheep...\n", sheep_count++);
        sleep(1);
    }

    return 0;
}
