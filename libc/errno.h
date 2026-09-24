#ifndef _ERRNO_H
#define _ERRNO_H

int *__errno(void);
#define errno (*__errno())

#define EPERM   1
#define ENOENT  2
#define EINTR   4
#define EIO     5
#define EBADF   9
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EEXIST 17
#define EINVAL 22
#define ENOSPC 28
#define ERANGE 34
#define ENOSYS 38

#define ESRCH 3
#define E2BIG 7
#define ENOEXEC 8
#define ECHILD 10
#define EFAULT 14
#define EBUSY 16
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define ENFILE 23
#define EMFILE 24
#define ENOTTY 25
#define EFBIG 27
#define ESPIPE 29
#define EROFS 30
#define EMLINK 31
#define EPIPE 32
#define EDOM 33
#define ENAMETOOLONG 36
#define ENOTEMPTY 39
#define EOVERFLOW 75
#define ENOTSUP 95

#endif
