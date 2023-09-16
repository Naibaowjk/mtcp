#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#ifdef DFUNCT
#define MEASURE_START() \
    uint64_t _start_time_ns; \
    { \
        struct timespec ts; \
        clock_gettime(CLOCK_MONOTONIC, &ts); \
        _start_time_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec; \
    }
#define MEASURE_END(label) \
    { \
        struct timespec ts; \
        clock_gettime(CLOCK_MONOTONIC, &ts); \
        uint64_t _end_time_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec; \
        printf("%s - %lu nsec\n", label, _end_time_ns - _start_time_ns); \
    }

#else
#define MEASURE_START() 
#define MEASURE_END(label) (void)0
#endif


#ifdef DCONFIG
#define TRACE_CONFIG(f, m...) fprintf(stdout, f, ##m)
#else
#define TRACE_CONFIG(f, m...) (void)0
#endif

#ifdef DBGERR

#define TRACE_ERROR(f, m...) { \
	fprintf(stdout, "[%10s:%4d] " f, __FUNCTION__, __LINE__, ##m);	\
	}

#else

#define TRACE_ERROR(f, m...)	(void)0

#endif /* DBGERR */

#ifdef DBGMSG

#define TRACE_DBG(f, m...) {\
	fprintf(stderr, "[%10s:%4d] " \
			f, __FUNCTION__, __LINE__, ##m);   \
	}

#else

#define TRACE_DBG(f, m...)   (void)0

#endif /* DBGMSG */

#ifdef INFO                       

#define TRACE_INFO(f, m...) {                                         \
	fprintf(stdout, "[%10s:%4d] " f,__FUNCTION__, __LINE__, ##m);    \
    }

#else

#define TRACE_INFO(f, m...) (void)0

#endif /* INFO */

#ifdef EPOLL
#define TRACE_EPOLL(f, m...) TRACE_FUNC("EPOLL", f, ##m)
#else
#define TRACE_EPOLL(f, m...)   (void)0
#endif

#ifdef APP
#define TRACE_APP(f, m...) TRACE_FUNC("APP", f, ##m)
#else
#define TRACE_APP(f, m...) (void)0
#endif

#ifdef DBGFUNC

#define TRACE_FUNC(n, f, m...) {                                         \
	fprintf(stderr, "%6s: %10s:%4d] " \
			f, n, __FUNCTION__, __LINE__, ##m);    \
	}

#else

#define TRACE_FUNC(f, m...) (void)0

#endif /* DBGFUNC */

#endif /* __DEBUG_H_ */
