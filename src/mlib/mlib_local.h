/*
 * local.h
 *
 *  Created on: Jan 4, 2023
 *      Author: amneiht
 */

#ifndef MLIB_MLIB_LOCAL_H_
#define MLIB_MLIB_LOCAL_H_

#include <mlib/base.h>
#include <mlib/module.h>
#include <pjlib-util/json.h>

/* create pj_str_t from const string  */
#define convert_str(a,b) pj_str_t* a = alloca(sizeof(pj_str_t)) ; \
a->slen = strlen(b); \
a->ptr = alloca(a->slen+1); \
a->ptr[a->slen]	= 0 ;\
pj_memcpy(a->ptr,b,a->slen)

#ifndef MLIB_MODULE_TMP_POOL
#define MLIB_MODULE_TMP_POOL 0
#endif

#define mlib_align(size , byte) ( (size%byte)?(size + byte -size%byte):size)
/* mlib */

MLIB_LOCAL mlib_module_t* _mlib_mod();

/* tree funtion */
MLIB_LOCAL pj_json_elem* _mlib_json_clone(pj_pool_t *pool,
		const pj_json_elem *sl);
typedef void (*tree_handle)(void *node, void *user_data);

/* mem */
#define mem_size sizeof(struct mem_header)

typedef struct mem_header {
#if MLIB_MEM_DEBUG == 1
	PJ_DECL_LIST_MEMBER(struct mem_header)
	;
	const char *func;
	int line;
#endif
	pj_bool_t des;
	int ref;
	mlib_clear_callback clear;

} mem_header;

#define mlib_mem_init_header(data, clear) mlib_mem_init_header_imp(MLIB_FUNC , MLIB_LINE , data , clear) ;

MLIB_LOCAL void mlib_mem_init_header_imp(const char *funt, int line, void *data,
		mlib_clear_callback clear);

MLIB_LOCAL int mlib_module_count();

#endif /* MLIB_MLIB_LOCAL_H_ */
