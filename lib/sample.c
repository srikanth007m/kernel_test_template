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
	int i;
	int nr = 2;
	char c;
	char *p;
	int mapflag = MAP_ANONYMOUS|MAP_SHARED;
	int protflag = PROT_READ|PROT_WRITE;
	int reserveonly = 0;
	unsigned long memsize = 0;
	unsigned long hpsizes[16]; /* possible hugepage size */
	int nr_hpsize = 0;

	while ((c = getopt(argc, argv, "h:rm:p:n:v")) != -1) {
		switch(c) {
                case 'h':
                        HPS = strtoul(optarg, NULL, 10) * 1024;
                        /* todo: arch independent */
                        if (HPS != 2097152 && HPS != 1073741824)
                                errmsg("Invalid hugepage size\n");
			mapflag |= MAP_HUGETLB;
			nr_hpsize = get_hugepage_sizes(hpsizes);
			for (i = 0; i < nr_hpsize; i++)
				if (HPS == hpsizes[i])
					break;
			if (i == nr_hpsize) /* not found */
				errmsg("invalid hugepage size\n");
                        break;
		case 'r': /* just reserve */
			reserveonly = 1;
			break;
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
		case 'v':
			verbose = 1;
			break;
		}
	}

	Dprintf("HPS = %x\n", HPS);
	signal(SIGUSR1, sig_handle);
	if (HPS > 0) {
		p = checked_mmap((void *)ADDR_INPUT, nr * HPS, protflag, mapflag, -1, 0);
		memsize = nr * HPS;
	} else {
		p = checked_mmap((void *)ADDR_INPUT, nr * PS, protflag, mapflag, -1, 0);
		memsize = nr * PS;
	}
	if (!reserveonly)
		memset(p, 'a', memsize);
	signal(SIGUSR1, sig_handle_flag);
	pprintf("busy loop to check pageflags\n");
	while (flag) {
		usleep(1000);
		if (!reserveonly)
			memset(p, 'a', memsize);
	}
	pprintf("%s exit\n", argv[0]);
	pause();
	exit(EXIT_SUCCESS);
}
