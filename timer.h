#pragma once


struct htimer_s;

typedef struct htimer_s htimer_t;
typedef void(*htimer_cb_t)(struct htimer_s* handle);

struct htimer_s
{
	void* heap_node[3];
	int active;
	uint64_t timeout;
	uint64_t repeat;
	uint64_t start_id;
	htimer_cb_t timer_cb;
	void* data;
};

void timer_init(htimer_t* handle);
int timer_start(htimer_t* handle,
				htimer_cb_t cb,
				uint64_t timeout,
				uint64_t repeat);
int timer_stop(htimer_t* handle);
int timer_again(htimer_t* handle);
void timer_set_repeat(htimer_t* handle, uint64_t repeat);
uint64_t timer_get_repeat(const htimer_t* handle);
void run_timers();