#pragma once
#include <time.h>
#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
#ifdef _WIN64
typedef long long          ssize_t;
#else
typedef long              ssize_t;
#endif
# define SSIZE_MAX INTPTR_MAX
# define _SSIZE_T_
# define _SSIZE_T_DEFINED
#endif
struct timezone {
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};

#include <WinSock2.h>


typedef unsigned char clockid_t;
void     InitTimeFunctions();
extern int      gettimeofday(struct timeval* tv, struct timezone* tz);
extern int clock_getres(clockid_t clock_id, struct timespec* res);
extern int clock_gettime(clockid_t clock_id, struct timespec* tp);