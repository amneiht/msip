/*
 * mlib_timer.h
 *
 *  Created on: Jan 8, 2023
 *      Author: amneiht
 */

/*
 * timer.h
 *
 *  Created on: Aug 19, 2022
 *      Author: amneiht
 */

#ifndef MLIB_TIMER_H_
#define MLIB_TIMER_H_

#include <mlib/module.h>
#include <pj/types.h>

typedef struct mlib_timer_controler mlib_timer_c; // timer controller
typedef struct mlib_timer_type mlib_timer_t;

mlib_timer_c* mlib_timer_control_create(mlib_module_t *mod, pj_int32_t ms);

// add callback when timer control is destroy
void mlib_timer_control_add_callback(mlib_timer_c *timer,
		mlib_clear_callback clear, void *data);

void mlib_timer_control_destroy(mlib_timer_c *timer);
/**
 * triger all time handle int timer_controler if you done want create a new thread
 * @param control timerc controller
 */
void mlib_timer_poll(mlib_timer_c *control);

/// loop the timer
void mlib_timer_loop(mlib_timer_c *control, void *user_data,
		pj_bool_t (*is_stop)(void *data));
/**
 * get the waiting time by ms in timer loop
 */
pj_int64_t mlib_timer_get_time(mlib_timer_c*);

/*timer entry funtion */
/**
 * create timer entry
 * @param pool data pool
 * @param name name of timer for debug
 * @param timer_func call when timer is trigger
 * @param user_data
 * @return
 */
mlib_timer_t* mlib_timer_entry_create(mlib_module_t *mod, char *name,
		pj_bool_t is_loop, pj_int32_t delay,
		void (*timer_func)(mlib_timer_t *entry, void *data), void *user_data);

void mlib_timer_entry_setloop(mlib_timer_t *entry, pj_bool_t is_loop);
pj_bool_t mlib_timer_entry_isloop(mlib_timer_t *entry);
void mlib_timer_entry_change_trigger_time(mlib_timer_t *entry, pj_int32_t delay);
pj_int32_t mlib_timer_entry_get_trigger_time(mlib_timer_t *entry);
pj_bool_t mlib_timer_entry_is_active(mlib_timer_t *entry);
/*end timer entry funtion */

/**
 * register new timer to timer controller
 * @param control
 * @param type
 * @return
 */
void mlib_timer_register(mlib_timer_c *control, mlib_timer_t *type);

/**
 * unregister new timer to timer controller
 */
void mlib_timer_unregister(mlib_timer_t *timer_entry);

#endif /* ACORE_TIMER_H_ */
