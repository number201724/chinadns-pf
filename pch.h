#ifndef _PCH_H_INCLUDED_
#define _PCH_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <ctype.h>
#include <err.h>
#include <pthread.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/wait.h>

#ifdef __linux__
#include <sys/epoll.h>
#elif defined(__FreeBSD__)
#include <sys/event.h>
#endif



#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_EVENTS 1024

#endif
