#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <getopt.h>
#include "include.h"

#define ADDR_INPUT 0x700000000000

int flag = 1;

void sig_handle(int signo) { ; }
void sig_handle_flag(int signo) { flag = 0; }

int main(int argc, char *argv[]) {
	int nr = 2;
	char c;
	char *p;
	int mapflag = MAP_ANONYMOUS|MAP_HUGETLB;
	int protflag = PROT_READ|PROT_WRITE;

	while ((c = getopt(argc, argv, "m:p:n:")) != -1) {
		switch(c) {
		case 'm':
			if (!strcmp(optarg, "private"))
				mapflag |= MAP_PRIVATE;
			else if (!strcmp(optarg, "shared"))
				mapflag |= MAP_SHARED;
			else
				errmsg("invalid optarg for -m\n");
			break;
		case 'p':
			testpipe = optarg;
			{
				struct stat stat;
				lstat(testpipe, &stat);
				if (!S_ISFIFO(stat.st_mode))
					errmsg("Given file is not fifo.\n");
			}
			break;
		case 'n':
			nr = strtoul(optarg, NULL, 10);
			break;
		}
	}

	signal(SIGUSR1, sig_handle);
	p = checked_mmap((void *)ADDR_INPUT, nr * HPS, protflag, mapflag, -1, 0);
	memset(p, 'a', nr * HPS);
	signal(SIGUSR1, sig_handle_flag);
	pprintf("busy loop to check pageflags\n");
	while (flag) {
		usleep(1000);
		memset(p, 'a', nr * HPS);
	}
	pprintf("%s exit\n", argv[0]);
	pause();
	exit(EXIT_SUCCESS);
}
