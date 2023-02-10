/*
 * mbase.c
 *
 *  Created on: Jan 4, 2023
 *      Author: amneiht
 */

#include <mlib/base.h>
#include <mlib_local.h>
#include <pjlib-util.h>
#include <pjlib.h>

static pj_caching_pool mlib_cp;
static pj_bool_t core_init = PJ_FALSE;
static mlib_module_t *main_module;

extern MLIB_LOCAL void mlib_module_init();
extern MLIB_LOCAL void mlib_module_close();

#if MLIB_MEM_DEBUG == 1
extern MLIB_LOCAL void mem_init_debug();
#endif
pj_status_t mlib_init() {
	if (core_init)
		return 0;
	core_init = PJ_TRUE;
	pj_status_t init = pj_init();
	pj_log_set_level(3);
	if (init != PJ_SUCCESS)
		return init;
	init = pjlib_util_init();
	if (init != PJ_SUCCESS)
		return init;
	//init memory pool
	pj_caching_pool_init(&mlib_cp, &pj_pool_factory_default_policy, 102400);
	pj_time_val tm;
	pj_gettimeofday(&tm);
	pj_srand(tm.sec);
	mlib_module_init();
	pj_str_t name = pj_str("mlib");
	main_module = mlib_module_simple(&name);
	// for memdebug
#if MLIB_MEM_DEBUG == 1
	mem_init_debug();
#endif

	return 0;

}
void mlib_close() {
	mlib_module_close();
//	_mlib_mem_close();
	pj_shutdown();
	core_init = PJ_FALSE;
}
pj_pool_t* mlib_pool_create(const char *name, int pool_size, int pool_inc) {
	return pj_pool_create(&mlib_cp.factory, name, pool_size, pool_inc, NULL);
}
void mlib_pool_release(pj_pool_t *pool) {
	pj_pool_release(pool);
}
pj_pool_factory* mlib_pool_factory() {
	return &mlib_cp.factory;
}
MLIB_LOCAL mlib_module_t* _mlib_mod() {
	return main_module;
}
