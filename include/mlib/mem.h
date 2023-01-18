/*
 * mlib_mem.h
 *
 *  Created on: Jan 3, 2023
 *      Author: amneiht
 */

#ifndef MLIB_MEM_H_
#define MLIB_MEM_H_

#include <mlib/base.h>
/**
 * bind pointer for memory control
 * @param pool pool to alloc data if
 * @param data pointer to data use wand to bind
 * @param clear clear call back
 * reference counter is 0
 * @return
 */
pj_status_t mlib_mem_bind(pj_pool_t *pool, void *data,
		mlib_clear_callback clear);
/**
 * check data is areadly bind to memory control
 * @param data
 * @return
 */
pj_bool_t mlib_mem_is_bind(void *data);
/**
 * add one to reference counter
 * @param data
 * @return
 */
pj_status_t mlib_mem_add_ref(void *data);
/**
 * decrease reffer to one
 * clear call_back will call when reference couter is -1
 * @param data
 * @return
 */
pj_status_t mlib_mem_dec_ref(void *data);

/**
 * check pointer if available to use
 * use must check it before add ref counter
 * @param data
 * @return
 */
pj_bool_t mlib_mem_is_available(void *data);

/**
 * Mark the memory to be destroyed, the  mlib_mem_is_available function
 * always returns false value when it call
 * @param data
 * @return
 */
pj_status_t mlib_mem_mask_destroy(void *data);

/**
 * bind pointer for memory control
 * @param pool pool to alloc data if
 * @param data pointer to data use wand to bind
 * @param clear cllear call back
 * @return
 */
pj_status_t mlib_mem_isbind(void *data);

/// get number of reference counter in data
int mlib_mem_get_nref(void *data);

#endif /* MLIB_MEM_H_ */
