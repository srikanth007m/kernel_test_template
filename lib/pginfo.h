#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

struct pagestat {
	unsigned long pfn;
	unsigned long pflags;
	unsigned long pcount;
};

unsigned long get_pfn(unsigned long vaddr, int pid);
unsigned long get_kpflags(unsigned long pfn);
long get_pcount(unsigned long pfn);

static int pagemap_fd;
static int kpageflags_fd;
static int kpagecount_fd;
static int hwpoison_inject_fd;
static int soft_offline_fd;

/* pagemap kernel ABI bits */
#define PM_ENTRY_BYTES      sizeof(uint64_t)
#define PM_STATUS_BITS      3
#define PM_STATUS_OFFSET    (64 - PM_STATUS_BITS)
#define PM_STATUS_MASK      (((1LL << PM_STATUS_BITS) - 1) << PM_STATUS_OFFSET)
#define PM_STATUS(nr)       (((nr) << PM_STATUS_OFFSET) & PM_STATUS_MASK)
#define PM_PSHIFT_BITS      6
#define PM_PSHIFT_OFFSET    (PM_STATUS_OFFSET - PM_PSHIFT_BITS)
#define PM_PSHIFT_MASK      (((1LL << PM_PSHIFT_BITS) - 1) << PM_PSHIFT_OFFSET)
#define PM_PSHIFT(x)        (((u64) (x) << PM_PSHIFT_OFFSET) & PM_PSHIFT_MASK)
#define PM_PFRAME_MASK      ((1LL << PM_PSHIFT_OFFSET) - 1)
#define PM_PFRAME(x)        ((x) & PM_PFRAME_MASK)

#define PM_PRESENT          PM_STATUS(4LL)
#define PM_SWAP             PM_STATUS(2LL)

static unsigned long do_u64_read(int fd, char *name,
				 uint64_t *buf,
				 unsigned long index,
				 unsigned long count)
{
        long bytes;

        if (index > ULONG_MAX / 8) {
                fprintf(stderr, "index overflow: %lu\n", index);
		exit(EXIT_FAILURE);
	}

        if (lseek(fd, index * 8, SEEK_SET) < 0) {
                perror(name);
                exit(EXIT_FAILURE);
        }

        if ((bytes = read(fd, buf, count * 8)) < 0) {
                perror(name);
                exit(EXIT_FAILURE);
        }

        if (bytes % 8) {
                fprintf(stderr, "partial read: %lu bytes\n", bytes);
		exit(EXIT_FAILURE);
	}

        return bytes / 8;
}

static unsigned long pagemap_read(uint64_t *buf,
				  unsigned long index,
				  unsigned long pages) {
	return do_u64_read(pagemap_fd, "/proc/pid/pagemap", buf, index, pages);
}

static unsigned long kpageflags_read(uint64_t *buf,
				    unsigned long index,
				    unsigned long pages) {
	return do_u64_read(kpageflags_fd, "/proc/kpageflags", buf, index, pages);
}

static unsigned long kpagecount_read(uint64_t *buf,
				    unsigned long index,
				    unsigned long pages) {
	return do_u64_read(kpagecount_fd, "/proc/kpagecount", buf, index, pages);
}

static unsigned long pagemap_pfn(uint64_t val) {
        unsigned long pfn;

        if (val & PM_PRESENT)
                pfn = PM_PFRAME(val);
        else
                pfn = 0;

        return pfn;
}

unsigned long get_pfn(unsigned long vaddr, int pid) {
	char file[128];
	uint64_t pfn[1];
	unsigned long index = ((unsigned long)vaddr) / getpagesize();

	if (!pid)
		pid = getpid();
	sprintf(file, "/proc/%d/pagemap", pid);
	if ((pagemap_fd = open(file, O_RDONLY)) < 0) {
		perror(file);
		exit(EXIT_FAILURE);
	}
	if (!pagemap_read(pfn, index, 1)) {
		perror("pagemap_read");
		exit(EXIT_FAILURE);
	}
	close(pagemap_fd);
	return pagemap_pfn(pfn[0]);
}

unsigned long get_kpflags(unsigned long pfn) {
	uint64_t kpflags[1];

	if ((kpageflags_fd = open("/proc/kpageflags", O_RDONLY)) < 0) {
		perror("reading /proc/kpageflags");
		exit(EXIT_FAILURE);
	}
	if (!kpageflags_read(kpflags, pfn, 1)) {
		perror("kpageflags_read");
		exit(EXIT_FAILURE);
	}
	close(kpageflags_fd);
	return (unsigned long)kpflags[0];
}

long get_pcount(unsigned long pfn) {
	uint64_t pcount[1];

	if ((kpagecount_fd = open("/proc/kpagecount", O_RDONLY)) < 0) {
		perror("reading /proc/kpagecount");
		exit(EXIT_FAILURE);
	}
	if (!kpagecount_read(pcount, pfn, 1)) {
		perror("kpagecount_read");
		exit(EXIT_FAILURE);
	}
	close(kpagecount_fd);
	return (long)pcount[0];
}

int check_kpflag(unsigned long vaddr, unsigned long kpf) {
	unsigned long pfn = get_pfn(vaddr, 0);
	unsigned long kpflags = get_kpflags(pfn);
	return kpflags | kpf;
}
