#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "heap-inl.h"
#include "timer.h"
#include "realtime.h"

#undef container_of
#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))


static struct heap g_timer_heap;
static uint64_t loop_time = 0;
static uint64_t timer_counter = 0;
static int g_timer_heap_initialized = 0;

//
// Purpose: 
//
static struct heap *timer_heap()
{
	if (!g_timer_heap_initialized)
	{
		heap_init(&g_timer_heap);
		g_timer_heap_initialized = 1;
	}

	return &g_timer_heap;
}

//
// Purpose: 
//
static int timer_less_than(const struct heap_node* ha,
						   const struct heap_node* hb)
{
	const htimer_t* a;
	const htimer_t* b;

	a = container_of(ha, htimer_t, heap_node);
	b = container_of(hb, htimer_t, heap_node);

	if (a->timeout < b->timeout)
		return 1;
	if (b->timeout < a->timeout)
		return 0;

	/* Compare start_id when both have the same timeout. start_id is
	 * allocated with loop->timer_counter in uv_timer_start().
	 */
	if (a->start_id < b->start_id)
		return 1;
	if (b->start_id < a->start_id)
		return 0;

	return 0;
}

//
// Purpose: 
//
void timer_init(htimer_t* handle)
{
	handle->active = 0;
	handle->timer_cb = NULL;
	handle->repeat = 0;
}

//
// Purpose: 
//
int timer_start(htimer_t* handle,
				htimer_cb_t cb,
				uint64_t timeout,
				uint64_t repeat)
{
	uint64_t clamped_timeout;

	if (cb == NULL)
		return -1;

	if (handle->active)
		timer_stop(handle);

	clamped_timeout = loop_time + timeout;
	if (clamped_timeout < timeout)
		clamped_timeout = (uint64_t)-1;

	handle->timer_cb = cb;
	handle->timeout = clamped_timeout;
	handle->repeat = repeat;
	/* start_id is the second index to be compared in uv__timer_cmp() */
	handle->start_id = timer_counter++;

	heap_insert(timer_heap(),
				(struct heap_node*)&handle->heap_node,
				timer_less_than);

	handle->active = 1;

	return 0;
}

//
// Purpose: 
//
int timer_stop(htimer_t* handle)
{
	if (!handle->active)
		return 0;

	heap_remove(timer_heap(),
				(struct heap_node*)&handle->heap_node,
				timer_less_than);

	handle->active = 0;
	return 0;
}

//
// Purpose: 
//
int timer_again(htimer_t* handle)
{
	if (handle->timer_cb == NULL)
		return -1;

	if (handle->repeat)
	{
		timer_stop(handle);
		timer_start(handle, handle->timer_cb, handle->repeat, handle->repeat);
	}

	return 0;
}

//
// Purpose: 
//
void timer_set_repeat(htimer_t* handle, uint64_t repeat)
{
	handle->repeat = repeat;
}


//
// Purpose: 
//
uint64_t timer_get_repeat(const htimer_t* handle)
{
	return handle->repeat;
}


//
// Purpose: 
//
void run_timers()
{
	struct heap_node* heap_node;
	htimer_t* handle;

	uint64_t new_time = (uint64_t)GetTime();

	assert(new_time >= loop_time);
	loop_time = new_time;

	for (;;)
	{
		heap_node = heap_min(timer_heap());
		if (heap_node == NULL)
			break;

		handle = container_of(heap_node, htimer_t, heap_node);
		if (handle->timeout > loop_time)
			break;

		timer_stop(handle);
		timer_again(handle);
		handle->timer_cb(handle);
	}
}
