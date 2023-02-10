/*
 * time_control.c
 *
 *  Created on: Aug 19, 2022
 *      Author: amneiht
 */

#include <mlib/base.h>
#include <mlib/timer.h>
#include <mlib/mem.h>
#include <mlib_local.h>
#include <pjlib.h>
struct mlib_timer_controler {
	pj_timer_heap_t *ht;
	pj_lock_t *lock;
	pj_bool_t time_change;
	pj_int32_t delay;
	// optional
	mlib_clear_callback clear;
	void *data;
};
struct mlib_timer_type {
	pj_timer_entry entry;
	pj_str_t name;
	pj_bool_t is_loop;
	pj_timer_heap_t *ht;
	pj_uint64_t triger_time;
	pj_time_val last_triger;
	void (*timer_func)(struct mlib_timer_type *entry, void *data);
	void *data;
};

static void timer_callback(pj_timer_heap_t *timer_heap,
		struct pj_timer_entry *entry) {
	mlib_timer_t *mlib = entry->user_data;
	mlib->timer_func(mlib, mlib->data);
	// count ting jitter
	if (!mlib->is_loop) {
		mlib->last_triger.sec = 0;
		return;
	}
	pj_time_val delay, now;
	// giam so lan tinh toan
	delay.sec = (mlib->triger_time) / 1000;
	delay.msec = (mlib->triger_time) % 1000;
	pj_gettimeofday(&now);
	PJ_TIME_VAL_ADD(mlib->last_triger, delay);
	PJ_TIME_VAL_ADD(delay, mlib->last_triger);
	PJ_TIME_VAL_SUB(delay, now);

	if (PJ_TIME_VAL_MSEC(delay) < 0) {
		// vuot qua time out
		PJ_LOG(5,
				(MLIB_NAME, "\"%.*s\" has handle time is more than delay",(int)mlib->name.slen , mlib->name.ptr));
		delay.sec = (mlib->triger_time) / 1000;
		delay.msec = (mlib->triger_time) % 1000;
		mlib->last_triger = now;
	}
	pj_timer_heap_schedule(timer_heap, entry, &delay);
}

static void timeheap_destroy(void *timer_heap) {
	mlib_timer_c *timer = timer_heap;
	if (timer->clear)
		timer->clear(timer->data);
	pj_timer_heap_destroy(timer->ht);
}
mlib_timer_c* mlib_timer_control_create(mlib_module_t *mod, pj_int32_t ms) {
	mlib_timer_c *heap = mlib_modctl_alloc(mod, sizeof(mlib_timer_c),
			timeheap_destroy);
	pj_pool_t *pool = mlib_modctl_pool(heap);
	if (ms == 0)
		ms = 100;
	heap->delay = ms;
	heap->time_change = pj_lock_create_simple_mutex(pool, "heap_lock",
			&heap->lock);
	pj_timer_heap_create(pool, 32, &heap->ht);
	pj_timer_heap_set_lock(heap->ht, heap->lock, PJ_TRUE);
	return heap;
}
void mlib_timer_control_destroy(mlib_timer_c *timer) {
	mlib_mem_mask_destroy((struct mlib_module_ctl*) timer);
}
void mlib_timer_control_add_callback(mlib_timer_c *timer,
		mlib_clear_callback clear, void *data) {
	timer->clear = clear;
	timer->data = data;
}
void mlib_timer_poll(mlib_timer_c *control) {
	pj_time_val tm;
	tm.sec = control->delay / 1000;
	tm.msec = control->delay % 1000;
	if (pj_timer_heap_count(control->ht) > 0)
		pj_timer_heap_poll(control->ht, &tm);
}
void mlib_timer_loop(mlib_timer_c *control, void *user_data,
		pj_bool_t (*is_stop)(void *data)) {
	pj_time_val delay, jiter;
	delay.sec = control->delay / 1000;
	delay.msec = control->delay % 1000;
	control->time_change = PJ_FALSE;
	pj_time_val now, pre;
	// precalate caculate for next poll time
	pj_gettimeofday(&pre);
	while (is_stop(user_data)) {
		if (control->time_change) {
			delay.sec = control->delay / 1000;
			delay.msec = control->delay % 1000;
			control->time_change = PJ_FALSE;
		}
		if (pj_timer_heap_count(control->ht) > 0)
			pj_timer_heap_poll(control->ht, &delay);
		pj_gettimeofday(&now);
		PJ_TIME_VAL_ADD(pre, delay);
		jiter = pre;
		PJ_TIME_VAL_SUB(jiter, now);
		if (PJ_TIME_VAL_MSEC(jiter) < 0) {
			pre = now;
			PJ_TIME_VAL_ADD(pre, delay);
		} else
			pj_thread_sleep(PJ_TIME_VAL_MSEC(jiter));
	}

}
void mlib_timer_register(mlib_timer_c *control, mlib_timer_t *entry) {
	pj_time_val delay;
	delay.sec = entry->triger_time / 1000;
	delay.msec = entry->triger_time % 1000;

	entry->ht = control->ht;
	pj_gettimeofday(&entry->last_triger);
	pj_timer_heap_schedule(control->ht, &entry->entry, &delay);

}
void mlib_timer_entry_active(mlib_timer_t *entry) {
	if (entry->ht) {
		pj_time_val delay;
		delay.sec = entry->triger_time / 1000;
		delay.msec = entry->triger_time % 1000;
		pj_gettimeofday(&entry->last_triger);
		pj_timer_heap_schedule(entry->ht, &entry->entry, &delay);
	}
}
void mlib_timer_unregister(mlib_timer_t *timer_entry) {
	if (timer_entry->ht)
		pj_timer_heap_cancel(timer_entry->ht, &timer_entry->entry);
	timer_entry->last_triger.sec = 0;
	timer_entry->ht = NULL;
}

mlib_timer_t* mlib_timer_entry_create(mlib_module_t *mod, char *name,
		pj_bool_t is_loop, pj_int32_t delay,
		void (*timer_func)(mlib_timer_t *entry, void *data), void *user_data) {
	struct mlib_timer_type *entry = (struct mlib_timer_type*) mlib_modctl_alloc(
			mod, sizeof(struct mlib_timer_type),
			(mlib_clear_callback) mlib_timer_unregister);
	pj_timer_entry_init(&entry->entry, 0, entry, timer_callback);
	entry->is_loop = is_loop;
	entry->timer_func = timer_func;
	entry->data = user_data;
	entry->triger_time = delay;
	entry->last_triger.sec = 0;
	pj_pool_t *pool = mlib_modctl_pool(entry);
	if (name != NULL) {
		pj_strdup2(pool, &entry->name, name);
	} else {
		pj_strdup2(pool, &entry->name, "timer_entry");
	}
	return entry;
}
void mlib_timer_entry_setloop(mlib_timer_t *entry, pj_bool_t is_loop) {
	entry->is_loop = is_loop;
}
pj_bool_t mlib_timer_entry_isloop(mlib_timer_t *entry) {
	return entry->is_loop;
}
void mlib_timer_entry_change_trigger_time(mlib_timer_t *entry, pj_int32_t delay) {
	entry->triger_time = delay;
}
pj_int32_t mlib_timer_entry_get_trigger_time(mlib_timer_t *entry) {
	return entry->triger_time;
}
pj_bool_t mlib_timer_entry_is_active(mlib_timer_t *entry) {
	return entry->last_triger.sec == 0;
}

