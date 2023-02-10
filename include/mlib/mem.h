/*
 * mlib_mem.h
 *
 *  Created on: Jan 3, 2023
 *      Author: amneiht
 */

#ifndef MLIB_MEM_H_
#define MLIB_MEM_H_

#include <mlib/base.h>

typedef void mlib_mmap_t;
// TODO chang to header alloc to control memory

/**
 * bind pointer for memory control
 * @param pool pool to alloc data if
 * @param data pointer to data use wand to bind
 * @param clear clear call back
 * reference counter is 0
 * @return
 */
#define mlib_mem_alloc(pool,size, clear) mlib_mem_debug_alloc(MLIB_FUNC , __LINE__ , pool,size,clear)

/**
 * check data is areadly bind to memory control
 * @param data
 * @return
 */

pj_status_t mlib_mem_add_ref(const void *data);
/**
 * decrease reffer to one
 * clear call_back will call when reference couter is -1
 * @param data
 * @return
 */
pj_status_t mlib_mem_dec_ref(const void *data);

/**
 * check pointer if available to use
 * use must check it before add ref counter
 * @param data
 * @return
 */
pj_bool_t mlib_mem_check(const void *data);

/**
 * Mark the memory to be destroyed, the  mlib_mem_is_available function
 * always returns false value when it call
 * @param data
 * @return
 */
void mlib_mem_release_list(pj_list *list);

// get number of reference counter in data
pj_status_t mlib_mem_mask_destroy(const void *data);

int mlib_mem_get_nref(const void *data);

//help for memry debug
void mlib_mem_dump(void *data, mlib_clear_callback clear);
void* mlib_mem_debug_alloc(const char *fun, int line, pj_pool_t *pool, int size,
		mlib_clear_callback clear);

mlib_mmap_t* mlib_mmap_create(pj_pool_t *pool);

// F(p1)=p2
pj_status_t mlib_mmap_set(pj_pool_t *pool, mlib_mmap_t *map, void *p1, void *p2);
// get p2 from p1
void* mlib_mmap_get(mlib_mmap_t *map, void *p1);

// get p2 from p1 abd remove to map
void* mlib_mmap_unset(mlib_mmap_t *map, void *p1);
#endif /* MLIB_MEM_H_ */
