/*
 * close() call that asyncronously flushes file from kernel page cache
 * mck - 12/27/13
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/syscall.h>

// default: proc/sys/vm/dirty_expire_centisecs
#define DEFAULT_LIFESPAN 30

int _close(int fd)
{
    return syscall(SYS_close, fd);
}

int _close_async(void *arg)
{
    int fd = (int)(long)arg;
    if (fd < 0)
        return -1;

    int lifespan = DEFAULT_LIFESPAN;
    char *ptr = getenv("CLOSEFLUSH_LIFESPAN");
    if (ptr != NULL) {
        lifespan = atoi(ptr);
        if (lifespan < 0)
            lifespan = 0;
    }

    daemon(0, 0);

    time_t time_start = time(NULL);

    sync_file_range(fd, 0, 0, SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE | SYNC_FILE_RANGE_WAIT_AFTER);

    time_t time_sync = time(NULL) - time_start;

    if (time_sync < lifespan) {
        struct timespec ts;
        ts.tv_sec = lifespan - time_sync;
        ts.tv_nsec = 0;
        nanosleep(&ts, NULL);
    }

    posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);

    _close(fd);
    return 0;
}

int close(int fd)
{
    void *stack = malloc(0x10000);
    if (stack != NULL)
        clone(_close_async, stack+0x10000, CLONE_PARENT, (void *)(long)fd);

    return _close(fd);
}

