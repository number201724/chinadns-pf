#include <stdint.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif
#include "realtime.h"

#ifndef _WIN32

typedef enum {
	CLOCK_PRECISE = 0,  /* Use the highest resolution clock available. */
	CLOCK_FAST = 1      /* Use the fastest clock with <= 1ms granularity. */
} uv_clocktype_t;

static uint64_t hrtime(uv_clocktype_t type) {
	static clock_t fast_clock_id = -1;
	struct timespec t;
	clock_t clock_id;
#ifndef __linux__
#define CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC_FAST
#endif
	/* Prefer CLOCK_MONOTONIC_COARSE if available but only when it has
	 * millisecond granularity or better.  CLOCK_MONOTONIC_COARSE is
	 * serviced entirely from the vDSO, whereas CLOCK_MONOTONIC may
	 * decide to make a costly system call.
	 */
	 /* TODO(bnoordhuis) Use CLOCK_MONOTONIC_COARSE for UV_CLOCK_PRECISE
	  * when it has microsecond granularity or better (unlikely).
	  */
	if (type == CLOCK_FAST && fast_clock_id == -1) {
		if (clock_getres(CLOCK_MONOTONIC_COARSE, &t) == 0 &&
			t.tv_nsec <= 1 * 1000 * 1000) {
			fast_clock_id = CLOCK_MONOTONIC_COARSE;
		}
		else {
			fast_clock_id = CLOCK_MONOTONIC;
		}
	}

	clock_id = CLOCK_MONOTONIC;
	if (type == CLOCK_FAST)
		clock_id = fast_clock_id;

	if (clock_gettime(clock_id, &t))
		return 0;  /* Not really possible. */

	return t.tv_sec * (uint64_t)1e9 + t.tv_nsec;
}

uint64_t GetTime()
{
	return hrtime(CLOCK_FAST) / 1000000;
}

#else

/* Interval (in seconds) of the high-resolution clock. */
static double hrtime_interval_ = 0;

uint64_t GetTime()
{
	LARGE_INTEGER counter;

	/* If the performance interval is zero, there's no support. */
	if (hrtime_interval_ == 0) {
		return 0;
	}

	if (!QueryPerformanceCounter(&counter)) {
		return 0;
	}

	/* Because we have no guarantee about the order of magnitude of the
	 * performance counter interval, integer math could cause this computation
	 * to overflow. Therefore we resort to floating point math.
	 */
	return (uint64_t)((double)counter.QuadPart * hrtime_interval_ * 1000);
}

void InitRealTime(void)
{
	LARGE_INTEGER perf_frequency;

	/* Retrieve high-resolution timer frequency
	 * and precompute its reciprocal.
	 */
	if (QueryPerformanceFrequency(&perf_frequency)) {
		hrtime_interval_ = 1.0 / perf_frequency.QuadPart;
	}
	else {
		hrtime_interval_ = 0;
	}
}

#endif
