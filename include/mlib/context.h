/*
 * mlib_context.h
 *
 *  Created on: Jan 3, 2023
 *      Author: amneiht
 */

#ifndef MLIB_CONTEXT_H_
#define MLIB_CONTEXT_H_

#include <mlib/user_config.h>
#include <pjlib.h>

typedef struct mlib_context_type mlib_context_t;
typedef struct mlib_context_list mlib_context_l;
typedef void mlib_context_val;

// context strlist
struct mlib_context_str_list {
	PJ_DECL_LIST_MEMBER(struct mlib_context_str_list )
	;
	pj_str_t value;
};
/* parse config file or buffer to context list*/
mlib_context_l* mlib_context_list_prase(pj_pool_t *pool, const pj_str_t *file);
mlib_context_l* mlib_context_list_prase2(pj_pool_t *pool, char *buffer,
		long len);

/* get context with specify name from context list*/
mlib_context_t* mlib_context_list_find(const mlib_context_l *list,
		const pj_str_t *name);
mlib_context_t* mlib_context_list_find2(const mlib_context_l *list,
		const char *name);
mlib_context_val* mlib_context_type_get_value(const mlib_context_t *ctx,
		const pj_str_t *val_name);
mlib_context_val* mlib_context_type_get_value2(const mlib_context_t *ctx,
		const char *val_name);

/*context value to type*/
const pj_str_t* mlib_context_val_to_str(const mlib_context_val *val);
int mlib_context_val_to_int(const mlib_context_val *val);
double mlib_context_val_to_double(const mlib_context_val *val);
pj_list* mlib_context_val_to_list(pj_pool_t *pool, const mlib_context_val *val);
pj_bool_t mlib_context_val_check(const mlib_context_val *val,
		const pj_str_t *value);

/*clone funtion*/
mlib_context_l* mlib_context_list_clone(pj_pool_t *pool,
		const mlib_context_t *ctx);

/* print funion*/
void mlib_conf_print(const mlib_context_l *conf);

/* search funtion */
const mlib_context_t* mlib_context_type_next(const mlib_context_t *context);
const pj_str_t* mlib_context_type_get_name(const mlib_context_t *context);
#endif /* MLIB_CONTEXT_H_ */
