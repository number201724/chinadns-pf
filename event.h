#ifndef _EVENT_H_INCLUDED_
#define _EVENT_H_INCLUDED_

typedef struct event_io_s
{
    uint32_t u32;
	int ident;
    int pevents;
}event_io_t;

void event_init(void);
void event_close(void);

// POLLIN
// POLLOUT
// POLLHUP
int event_add(int fd, event_io_t *w);
int event_ctl(int fd, bool enable, int events);
int event_del(int fd);

// void event_set(int fd, int filter, int flags, void *data);

void doevent(void);


extern int event_ident;
extern event_io_t **event_watchers;
extern unsigned int event_nwatchers;

#endif
