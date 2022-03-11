#ifndef _REALTIME_H_INCLUDED_
#define _REALTIME_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
void InitRealTime(void);
#else
#define InitRealTime()
#endif

uint64_t GetTime();

#ifdef __cplusplus
};
#endif

#endif