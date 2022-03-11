#include "pch.h"
#include "realtime.h"
#include "timer.h"
#include "event.h"


int event_ident;
event_io_t **event_watchers;
unsigned int event_nwatchers;
extern uint64_t realtime;

#define MAX_EVENTS 1024

//
// Purpose: 
//
static unsigned int next_power_of_two(unsigned int val)
{
	val -= 1;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	val += 1;
	return val;
}

//
// Purpose: 
//
void event_init(void)
{
	event_watchers = NULL;
	event_nwatchers = 0;

#ifdef __FreeBSD__
	event_ident = kqueue();
	if (event_ident == -1) {
		err(EXIT_FAILURE, "kqueue failed");
	}
#elif defined(__linux__)
	event_ident = epoll_create(1);
	if (event_ident == -1) {
		err(EXIT_FAILURE, "epoll_create failed");
	}
#endif
}

//
// Purpose: 
//
void event_close(void)
{
	if (event_ident != -1) {
		close(event_ident);
		event_ident = -1;
	}
}

//
// Purpose: 
//
static void maybe_resize(unsigned int len)
{
	unsigned int nwatchers;
	event_io_t **watchers;
	unsigned int i;

	if (len < event_nwatchers) {
		return;
	}

	nwatchers = next_power_of_two(len + 2) - 2;
	watchers = realloc(event_watchers, nwatchers * sizeof(event_io_t*));

	if (watchers == NULL) {
		err(EXIT_FAILURE, "realloc watchers failed");
	}

	for (i = event_nwatchers; i < nwatchers; i++)
		watchers[i] = NULL;

	event_watchers = watchers;
	event_nwatchers = nwatchers;
}


//
// Purpose: 
//
int event_add(int fd, event_io_t *w)
{
	int rc;
#ifdef __FreeBSD__
	struct kevent events[1];
#elif defined(__linux__)
	struct epoll_event events;
#endif
	maybe_resize(fd + 1);

	event_watchers[fd] = w;

	w->ident = fd;
	w->pevents = POLLIN;

#ifdef __FreeBSD__
	EV_SET(&events[0], w->ident, EVFILT_READ, EV_ENABLE | EV_ADD, 0, 0, w);
	rc = kevent(event_ident, events, 1, NULL, 0, NULL);
#elif defined(__linux__)
	memset(&events, 0, sizeof(events));
	events.data.fd = fd;
	events.events = EPOLLIN;
	rc = epoll_ctl(event_ident, EPOLL_CTL_ADD, fd, &events);
#endif

	return rc;
}

//
// Purpose: 
//
int event_ctl(int fd, bool enable, int events)
{
	event_io_t *w;
// 	int revents;
	int rc;
#ifdef __linux__
	struct epoll_event epv; 
#elif defined(__FreeBSD__)
	struct kevent kev[8];
	int n;
#endif

	assert((unsigned int)fd < event_nwatchers);

	if (event_watchers[fd] == NULL) {
		return -1;
	}

	w = event_watchers[fd];

	if (enable)
		w->pevents |= events;
	else
		w->pevents &= ~events;
	
#ifdef __linux__
	epv.data.fd = fd;
	epv.events = 0;

	if (w->pevents & POLLIN)
		epv.events |= EPOLLIN;

	if (w->pevents & POLLOUT)
		epv.events |= EPOLLOUT;

	
	rc = epoll_ctl(event_ident, EPOLL_CTL_MOD, w->ident, &epv);
#elif defined(__FreeBSD__)
	n = 0;
	
	if (enable) 
	{
		if(events & POLLIN) {
			EV_SET(&kev[n], w->ident, EVFILT_READ, EV_ENABLE | EV_ADD, 0, 0, w);
			n++;
		}

		if(events & POLLOUT) {
			EV_SET(&kev[n], w->ident, EVFILT_WRITE, EV_ENABLE | EV_ADD, 0, 0, w);
			n++;
		}
	}
	else
	{
		if(events & POLLIN) {
			EV_SET(&kev[n], w->ident, EVFILT_READ, EV_DISABLE, 0, 0, w);
			n++;
		}

		if(events & POLLOUT) {
			EV_SET(&kev[n], w->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, w);
			n++;
		}
	}

	rc = kevent(event_ident, kev, n, NULL, 0, NULL);
#endif
	return rc;
}

//
// Purpose: 
//
int event_del(int fd)
{
#ifdef __linux__
	struct epoll_event events;

	assert(fd < event_nwatchers);

	memset(&events, 0, sizeof(events));
	events.data.fd = -1;
	events.events = 0;

	event_watchers[fd] = NULL;

	return epoll_ctl(event_ident, EPOLL_CTL_DEL, fd, &events);
#elif defined(__FreeBSD__)
	struct kevent events[2];
	
	if (event_watchers[fd] == NULL)
		return 0;

	assert((unsigned int)fd < event_nwatchers);

	EV_SET(&events[0], fd, EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0, NULL);
	EV_SET(&events[1], fd, EVFILT_WRITE, EV_DISABLE | EV_DELETE, 0, 0, NULL);

	event_watchers[fd] = NULL;

	return kevent(event_ident, events, 2, NULL, 0, NULL);
	
#endif
}
