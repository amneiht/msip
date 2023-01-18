/*
 * control.c
 *
 *  Created on: Jan 16, 2023
 *      Author: amneiht
 */

#include <mlib/module.h>
#include <mlib/control.h>
#include <pj/list.h>
#include <pj/lock.h>
#include "../local.h"
/* control handle pointer*/
#define _c_lock(control)  pj_lock_acquire(control->lock); {
#define _c_rls(control)         } pj_lock_release(control->lock);

struct mlib_control_handle {
	PJ_DECL_LIST_MEMBER(struct mlib_control_handle)
	;
	pj_str_t name;
	int priority;
	mlib_control_t check;
	mlib_clear_callback clear;
	void *data;
};
/* control object*/
struct mlib_control_list {
	PJ_DECL_LIST_MEMBER(struct mlib_control_list)
	;
	pj_str_t name;
	pj_list list;
	mlib_clear_callback clear;
	pj_lock_t *lock;
	void *data;
};

/* control list funtion */
static void clear_list(void *arg) {
	mlib_control_list *list = arg;
	mlib_modctl_list_destroy(&list->list);
	pj_lock_destroy(list->lock);
}
mlib_control_list* mlib_control_list_create(mlib_module_t *mod,
		const pj_str_t *name) {
	mlib_control_list *list = mlib_modctl_alloc(mod,
			sizeof(struct mlib_control_list), clear_list);
	pj_pool_t *pool = mlib_modctl_pool(list);
	if (name != NULL) {
		pj_strdup(pool, &list->name, name);
	}
	pj_list_init(&list->list);
	pj_lock_create_recursive_mutex(pool, "pika", &list->lock);
	return list;

}
void mlib_control_list_add(mlib_control_list *control,
		mlib_control_handle *handle) {
	_c_lock(control);
	mlib_control_handle *frist, *last;
	frist = control->list.next;
	last = (mlib_control_handle*) &control->list;
	pj_bool_t set = PJ_TRUE;
	while (last != frist) {
		if (frist->priority >= handle->priority) {
			set = PJ_FALSE;
			pj_list_insert_before(frist, handle);
		}
	}
	if (set)
		pj_list_insert_after(last->prev, handle);
	_c_rls(control);
}
void mlib_control_list_release(mlib_control_list *list) {
	mlib_mem_mask_destroy(list);
}

/* return true when all check is true */
static int and_check(void *data, const void *node) {
	const mlib_control_handle *handle = node;
	mlib_container *mcon = data;
	if (handle->check(mcon, handle->data)) {
		return mlib_search_notfound;
	}
	return mlib_search_found;
}
pj_bool_t mlib_control_and_check(mlib_control_list *ctl,
		mlib_container *work_data) {
	pj_bool_t res = PJ_TRUE;
	_c_lock(ctl)
		void *node = pj_list_search(&ctl->list, work_data, and_check);
		if (node != NULL)
			res = PJ_FALSE;
	_c_rls(ctl)
	return res;
}
/* return true when al lease one check is true */
static int or_check(void *data, const void *node) {
	const mlib_control_handle *handle = node;
	mlib_container *mcon = data;
	if (handle->check(mcon, handle->data)) {
		return mlib_search_found;
	}
	return mlib_search_notfound;
}
pj_bool_t mlib_control_or_check(mlib_control_list *ctl,
		mlib_container *work_data) {
	pj_bool_t res = PJ_TRUE;
	_c_lock(ctl)
		void *node = pj_list_search(&ctl->list, work_data, or_check);
		if (node == NULL)
			res = PJ_FALSE;
	_c_rls(ctl)
	return res;
}

/* control handle funtion */
static void clear_handle(void *arg) {
	mlib_control_handle *handle = arg;
	if (handle->clear)
		handle->clear(handle->data);
}
mlib_control_handle* mlib_control_handle_create(mlib_module_t *mod,
		mlib_control_t ctl, int priority, void *user_data,
		mlib_clear_callback clear) {
	mlib_control_handle *handle = mlib_modctl_alloc(mod,
			sizeof(struct mlib_control_handle), clear_handle);
	handle->data = user_data;
	handle->clear = clear;
	handle->priority = priority;
	return handle;
}
void mlib_control_handle_release(mlib_control_handle *handle) {
	mlib_modctl_release(handle);
}
