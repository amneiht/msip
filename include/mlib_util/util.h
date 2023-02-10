/*
 * util.h
 *
 *  Created on: Jan 16, 2023
 *      Author: amneiht
 */

#ifndef MLIB_UTIL_H_
#define MLIB_UTIL_H_

#include <mlib/event.h>
#include <mlib/timer.h>
#include <mlib_util/ui.h>

typedef enum mlib_event {
	MLIB_CLOSE, // data is NULL
	MLIB_MAX
} mlib_event;

void mlib_util_event_add_handle(mlib_event_handle_t *handle);
void mlib_util_event_send(mlib_event evt);
void mlib_util_timer_register(mlib_timer_t *handle);

void mlib_loop();
#endif /* MLIB_UTIL_H_ */
