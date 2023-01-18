/*
 * mlib_module.h
 *
 *  Created on: Jan 3, 2023
 *      Author: amneiht
 */

#ifndef MLIB_MODULE_H_
#define MLIB_MODULE_H_

#include <pj/types.h>
#include <mlib/config.h>
#include <mlib/context.h>
#include <mlib/base.h>
typedef struct mlib_module_t mlib_module_t;

#define MLIB_MOD_DECL(load , unload , destroy) \
mlib_module_object module_mod {load , unload , destroy };

typedef struct {
	pj_status_t (*load)(mlib_module_t *ctl, void **data_ptr);
	void (*unload)(void *data);
	void (*destroy)(void *data);
} mlib_module_object;

mlib_module_t* mlib_module_load(const pj_str_t *name, const pj_str_t *path);
void mlib_module_unload(mlib_module_t *mod);
void mlib_module_conf(const mlib_context_l *ctx);

mlib_module_t* mlib_module_simple(const pj_str_t *name);

mlib_module_t* mlib_module_load2(const pj_str_t *name, const char *path);

void* mlib_module_add_control(mlib_module_object *mod, void *data,
		mlib_clear_callback clear);
void* mlib_module_add_control2(mlib_module_object *mod, void *data);
/* module alloc control */
/**
 * alloc data and bind it to module memory control
 * it is auto release when unload module
 * @param mctl
 * @param size
 * @param un_reg
 */
void* mlib_modctl_alloc(mlib_module_t *mctl, int size,
		mlib_clear_callback un_reg);
void mlib_modctl_list_destroy(pj_list *list);

pj_pool_t* mlib_modctl_pool(void *pointer);

#endif /* MLIB_MODULE_H_ */
