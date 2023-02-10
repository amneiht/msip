/*
 * timer_util.c
 *
 *  Created on: Jan 14, 2023
 *      Author: amneiht
 */

#include <mlib/timer.h>
#include <mlib_local.h>
#include <mlib_util/util.h>
static mlib_timer_c *main_timer;
static mlib_event_t *evt;
static pj_bool_t need_init = PJ_TRUE;

#define _lib_init_ if(need_init)  first_init();
static void lib_des(void *arg) {
	need_init = PJ_TRUE;
}
static void first_init() {
	mlib_module_t *mod = _mlib_mod();
	main_timer = mlib_timer_control_create(mod, 5000);
	mlib_timer_control_add_callback(main_timer, lib_des, NULL);
	evt = mlib_event_create(mod, "Main event");
	need_init = PJ_FALSE;
}
void mlib_util_event_add_handle(mlib_event_handle_t *handle) {
	_lib_init_
	mlib_event_register(evt, handle);
}

void mlib_util_timer_register(mlib_timer_t *handle) {
	_lib_init_
	mlib_timer_register(main_timer, handle);
}
void mlib_util_event_send(mlib_event evttype) {
	_lib_init_
	mlib_event_send(evt, evttype, NULL);
}
static pj_bool_t is_stop(void *arg) {
	pj_bool_t *res = arg;
	if (!*res) {
		if (mlib_module_count() <= 1)
			return PJ_FALSE;
	}
	return *res;
}
static void event_handle(void *user_data, int type, void *event_data) {
	pj_bool_t *res = user_data;
	if (type == MLIB_CLOSE) {
		*res = PJ_FALSE;
	}
}
void mlib_loop() {
	_lib_init_
	pj_bool_t loop = PJ_TRUE;
	mlib_event_handle_t *p = mlib_event_handle_create(_mlib_mod(), event_handle,
	NULL, &loop);
	mlib_event_register(evt, p);
	mlib_timer_loop(main_timer, &loop, is_stop);

}
