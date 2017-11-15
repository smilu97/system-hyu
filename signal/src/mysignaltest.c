#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

static jmp_buf env_alrm;

int doing_sig_int = 0;

static void sig_alrm(int signo);
static void sig_int(int signo);
unsigned int sleep2(unsigned int seconds);

void nothing() {}

int main(void)
{
	unsigned int unslept;

	if (signal(SIGINT, sig_int) == SIG_ERR){
		fprintf(stderr, "signal(SIGINT) error");
		exit(-1);
	}
	if (signal(SIGUSR1, nothing) == SIG_ERR) {
		fprintf(stderr, "signal(SIGUSR1) error");
		exit(-1);
	}
	
	unslept = sleep2(5);

	printf("sleep2 returned: %u\n", unslept);

	return 0;
}

static void sig_alrm(int signo)
{
	if(doing_sig_int) return;
	longjmp(env_alrm, 1);
}

static void sig_int(int signo)
{
	if(doing_sig_int) return; 

	int i,j;
	volatile int k;

	printf("\nsig_int starting\n");
	doing_sig_int = 1;
	
	/*
	 * 아래 for문이 5초 이상 실행되도록 적당히 바꿔주세요. 
	 */
	for (i = 0; i<100; i++) {
		for (j = 0; j<12000000; j++) {
			k += i*j;	
		}
		printf("sigin: %d\n", i);
	}
		
	
	printf("sig_int finished\n");
	doing_sig_int = 0;
}

unsigned int sleep2(unsigned int seconds)
{
	if (signal(SIGALRM, sig_alrm) == SIG_ERR)
		return seconds;

	if (setjmp(env_alrm) == 0){
		alarm(seconds);
		pause();
	}
	return alarm(0);
}
