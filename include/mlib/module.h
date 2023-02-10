/*
 * mlib_module.h
 *
 *  Created on: Jan 3, 2023
 *      Author: amneiht
 */

#ifndef MLIB_MODULE_H_
#define MLIB_MODULE_H_

#include <pj/types.h>
#include <mlib/context.h>
#include <mlib/base.h>
#include <mlib/user_config.h>
typedef struct mlib_module_t mlib_module_t;

#define MLIB_MOD_DECL(load , unload , destroy) \
mlib_module_object module_mod {load , unload , destroy };
enum MLIB_MODUE_STATE {
	MLIB_MODUE_STATE_CLOSE, MLIB_MODUE_STATE_RELOAD,
};
typedef void (*mlib_module_state_handle)(void *data,
		enum MLIB_MODUE_STATE state);

typedef struct {
	pj_status_t (*load)(mlib_module_t *ctl, void **data_ptr);
	void (*unload)(void *data);
	void (*destroy)(void *data);
} mlib_module_object;

mlib_module_t* mlib_module_load(const pj_str_t *name, const pj_str_t *path);
void mlib_module_unload(mlib_module_t *mod);
void mlib_module_conf(const mlib_context_l *ctx);

mlib_module_t* mlib_module_simple(const pj_str_t *name);

pj_pool_t* mlib_module_pool(mlib_module_t *mod);

//mlib_module_t* mlib_module_load2(const pj_str_t *name, const char *path);

void mlib_module_add_listen(mlib_module_t *mod, void *data,
		mlib_module_state_handle hand);
/**
 * Add callback when module already remove
 */
void mlib_module_add_callback(mlib_module_t *mod, void *data,
		mlib_clear_callback clear);
/* module alloc control */
/**
 * alloc data and bind it to module memory control
 * it is auto release when unload module
 * @param mctl
 * @param size
 * @param un_reg
 */
#define mlib_modctl_alloc(mctl, size, un_reg)  mlib_modctl_debug_alloc(MLIB_FUNC , __LINE__ , mctl , size , un_reg)

void* mlib_modctl_debug_alloc(const char *funt, int line, mlib_module_t *mctl,
		int size, mlib_clear_callback un_reg);

pj_pool_t* mlib_modctl_pool(void *pointer);
void mlib_modctl_release(void *pointer);

#endif /* MLIB_MODULE_H_ */
