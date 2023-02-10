/*
 * mlib_mem2.c
 *
 *  Created on: Feb 8, 2023
 *      Author: amneiht
 */

#include <mlib/mem.h>
#include <mlib/module.h>
#include <pjlib.h>
#include "mlib_local.h"

#if MLIB_MEM_DEBUG == 1
static pj_list mem_list;
static pj_lock_t *mlock;
static int show_leak(void *data, const void *node) {
	const mem_header *head = node;
	PJ_LOG(1,
			(MLIB_NAME , "memory leck on fuction :%s on line %d", head->func, head->line));
	return mlib_search_notfound;
}
static void end_call_back(void *arg) {
	pj_lock_destroy(mlock);
	pj_list_search(&mem_list, NULL, show_leak);

}
MLIB_LOCAL void mem_init_debug() {
	pj_list_init(&mem_list);
	pj_lock_create_simple_mutex(mlib_module_pool(_mlib_mod()), NULL, &mlock);
	mlib_module_add_callback(_mlib_mod(), NULL, end_call_back);
}

#endif

void* mlib_mem_debug_alloc(const char *fun, int line, pj_pool_t *pool, int size,
		mlib_clear_callback clear) {
	void *data = pj_pool_zalloc(pool, size + sizeof(struct mem_header));
	mlib_mem_init_header_imp(fun, line, data, clear);
	return data + sizeof(struct mem_header);
}
/**
 * check data is areadly bind to memory control
 * @param data
 * @return
 */

pj_status_t mlib_mem_add_ref(const void *data) {
	void *init = (void*) data - mem_size;
	mem_header *mh = init;
	mh->ref++;
	return PJ_TRUE;
}
static void clear_mem(mem_header *cmd, const void *data) {
#if MLIB_MEM_DEBUG == 1
	pj_lock_acquire(mlock);
	pj_list_erase(cmd);
	pj_lock_release(mlock);
#endif
	cmd->clear((void*) data);
}
/**
 * decrease reffer to one
 * clear call_back will call when reference couter is -1
 * @param data
 * @return
 */
pj_status_t mlib_mem_dec_ref(const void *data) {
	void *init = (void*) data - mem_size;
	mem_header *cmd = init;
	cmd->ref--;
	if (cmd->ref < 0) {
		clear_mem(cmd, data);
	} else if (cmd->ref == 0 && cmd->des) {
		clear_mem(cmd, data);
	}
	return PJ_TRUE;
}

/**
 * check pointer if available to use
 * use must check it before add ref counter
 * @param data
 * @return
 */
pj_bool_t mlib_mem_check(const void *data) {
	void *init = (void*) data - mem_size;
	mem_header *cmd = init;
	return !cmd->des;
}

/// get number of reference counter in data
pj_status_t mlib_mem_mask_destroy(const void *data) {
	void *init = (void*) data - mem_size;
	mem_header *cmd = init;
	cmd->des = PJ_TRUE;
	if (cmd->ref <= 0) {
		clear_mem(cmd, data);
	}
	return PJ_SUCCESS;
}

int mlib_mem_get_nref(const void *data) {
	void *init = (void*) data - mem_size;
	mem_header *cmd = init;
	return cmd->ref;
}

MLIB_LOCAL void mlib_mem_init_header_imp(const char *funt, int line, void *data,
		mlib_clear_callback clear) {
	mem_header *ls = data;
	ls->clear = clear;
	ls->des = 0;
	ls->ref = 0;
#if MLIB_MEM_DEBUG == 1
	pj_lock_acquire(mlock);
	ls->func = funt;
	ls->line = line;
	pj_list_insert_after(&mem_list, ls);
	pj_lock_release(mlock);
#endif

}

