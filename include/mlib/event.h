/*
 * mlib_event.h
 *
 *  Created on: Jan 4, 2023
 *      Author: amneiht
 */

#ifndef MLIB_EVENT_H_
#define MLIB_EVENT_H_

#include <mlib/module.h>
#include <mlib/base.h>
#include <pjlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mlib_event_handle_t mlib_event_handle_t;
typedef struct mlib_event_t mlib_event_t;

typedef void (*mlib_handle_p)(void *user_data, int type, void *event_data);

/// create custom event publicser for user
mlib_event_t* mlib_event_create(mlib_module_t *mod, const char *event_name);
// call close funtion all event handle
pj_status_t mlib_event_destroy(mlib_event_t *evt);

void mlib_event_register(mlib_event_t *evt, mlib_event_handle_t *handle);

mlib_event_handle_t* mlib_event_handle_create(mlib_module_t *mod,
		mlib_handle_p handle, mlib_clear_callback clear, void *data);

void mlib_event_unregister_handle(mlib_event_handle_t *handle);
void mlib_event_send(const mlib_event_t*, int type, void *data);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_EVENT_H_ */
