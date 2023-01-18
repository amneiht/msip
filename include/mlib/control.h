/*
 * control.h
 *
 *  Created on: Jan 16, 2023
 *      Author: amneiht
 */

#ifndef MLIB_CONTROL_H_
#define MLIB_CONTROL_H_

#include <mlib/module.h>
#include <pj/list.h>
typedef pj_bool_t (*mlib_control_t)(mlib_container *work_data, void *user_data);
/* control handle pointer*/
typedef struct mlib_control_handle mlib_control_handle;
/* control object*/
typedef struct mlib_control_list mlib_control_list;

/* control list funtion */
mlib_control_list* mlib_control_list_create(mlib_module_t *mod, const pj_str_t *name);
/**
 * add handle to the list , handle will auto call clear funtion in mlib_control_handle when list release
 * @param control
 * @param handle
 */
void mlib_control_list_add(mlib_control_list *control,
		mlib_control_handle *handle);
void mlib_control_list_release(mlib_control_list *list);



/* return true when all check is true */
pj_bool_t mlib_control_and_check(mlib_control_list *ctl,
		mlib_container *work_data);
/* return true when al lease one check is true */
pj_bool_t mlib_control_or_check(mlib_control_list *ctl,
		mlib_container *work_data);



/* control handle funtion */
mlib_control_handle* mlib_control_handle_create(mlib_module_t *mod,
		mlib_control_t ctl, int proty, void *user_data,
		mlib_clear_callback clear);
void mlib_control_handle_release(mlib_control_handle *handle);

#endif /* MLIB_CONTROL_H_ */
