/*
 * close() call that asyncronously flushes file from kernel page cache
 * mck - 12/16/13
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/syscall.h>

int _close(int fd)
{
    return syscall(SYS_close, fd);
}

int _close_async(void *arg)
{
    int fd = (int)(long)arg;
    // could add a small delay/lifespan here if perhaps another process might open this file again soon ...
    sync_file_range(fd, 0, 0, SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE | SYNC_FILE_RANGE_WAIT_AFTER);
    posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
    _close(fd);
    return 0;
}

int close(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if ((flags & O_ACCMODE) != O_RDONLY) { // perhaps better to use inotify to check for IN_MODIFY ...
        void *stack = mmap(NULL, 16384, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN, -1, 0);
        if (stack != MAP_FAILED)
            clone(_close_async, stack, CLONE_FS | CLONE_PARENT, (void *)(long)fd);
    }
    else
        posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
    return _close(fd);
}

