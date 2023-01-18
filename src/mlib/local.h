/*
 * local.h
 *
 *  Created on: Jan 4, 2023
 *      Author: amneiht
 */

#ifndef MLIB_LOCAL_H_
#define MLIB_LOCAL_H_

#include <mlib/base.h>
#include <mlib/module.h>
#include <pjlib-util/json.h>

/* create pj_str_t from const string  */
#define convert_str(a,b) pj_str_t* a = alloca(sizeof(pj_str_t)) ; \
a->slen = strlen(b); \
a->ptr = alloca(a->slen+1); \
a->ptr[a->slen]	= 0 ;\
pj_memcpy(a->ptr,b,a->slen)

#define mlib_align(size , byte) ( (size%byte)?(size + byte -size%byte):size)
/* mlib */

enum mlib_search_result {
	mlib_search_found = 0, mlib_search_notfound = 1
};

MLIB_LOCAL mlib_module_t* _mlib_mod();

/* tree funtion */
MLIB_LOCAL pj_json_elem* _mlib_json_clone(pj_pool_t *pool,
		const pj_json_elem *sl);
typedef void (*tree_handle)(void *node, void *user_data);

/* mem */
MLIB_LOCAL void _mlib_mem_init();
MLIB_LOCAL void _mlib_mem_close();

#endif /* MLIB_LOCAL_H_ */
