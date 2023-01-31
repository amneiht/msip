/*
 * mlib_module.c
 *
 *  Created on: Jan 4, 2023
 *      Author: amneiht
 */

#include <dlfcn.h>
#include <mlib/base.h>
#include <mlib/module.h>
#include <mlib/mem.h>
#include <mlib_local.h>
#include <pjlib.h>

static pj_list clist;
static pj_bool_t init = PJ_FALSE;
static mlib_context_t *conf;

#define _ptr(a) (int)a->slen , a->ptr

#if MLIB_MINI_SYSTEM
static pj_pool_t *mpool;
static pj_lock_t *module_lock;
#define _mlock(mod) pj_lock_acquire(module_lock); {
#define _mrls(mod)  } pj_lock_release(module_lock);
#else
#define _mlock(mod) pj_lock_acquire(mod->lock); {
#define _mrls(mod)  } pj_lock_release(mod->lock);
#endif

enum module_type {
	module_type_dynamic_load, // dynamic load module
	module_type_simple
};

typedef struct __attribute__((aligned(4))) mlib_module_ctl {
	PJ_DECL_LIST_MEMBER(struct mlib_module_ctl)
	;
	pj_pool_t *pool;
	mlib_module_t *mctl;
	mlib_clear_callback un_reg;
} mlib_module_ctl;

struct mod_release {
	PJ_DECL_LIST_MEMBER(struct mod_release)
	;
	mlib_clear_callback clear;
	void *data;
};
struct mlib_module_t {
	PJ_DECL_LIST_MEMBER(struct mlib_module_t)
	;
	pj_str_t name;
	int cnt_ref;
	pj_pool_t *pool;

#if MLIB_MINI_SYSTEM !=1
	pj_lock_t *lock;
#endif
	enum module_type type;

	struct {
		mlib_module_object *mod;
		void *mod_data;
		void *handle;
	} dynamic;
	pj_bool_t clear_lock;

	// clear callback opotion
//	struct {
//		PJ_DECL_LIST_MEMBER(struct mlib_module_ctl)
//		;
//	} list;
	pj_list list;
	pj_list rls;

};
/* funtion declere*/
static void ctl_release(struct mlib_module_ctl *ctl);

MLIB_LOCAL void mlib_module_init() {
	if (init)
		return;
	pj_list_init(&clist);
#if MLIB_MINI_SYSTEM == 1
	mpool = mlib_pool_create("mod", 2048, 2048);
	pj_lock_create_recursive_mutex(mpool, "module_lock", &module_lock);
#endif
	init = PJ_TRUE;
}

static int free_callback(void *data, const void *note) {
	(void) data;
	const struct mod_release *mod = note;
	mod->clear(mod->data);
	return mlib_search_notfound;
}

static void clear_mod(mlib_module_t *mod) {
	if (mod->cnt_ref <= 0) {
		pj_list_erase(mod);
#if MLIB_MINI_SYSTEM !=1
pj_lock_release(mod->lock);
#endif
		pj_list_search(&mod->rls, NULL, free_callback);
		if (mod->type == module_type_dynamic_load) {
			dlclose(mod->dynamic.handle);
		}
		mlib_pool_release(mod->pool);
	} else {
		mod->clear_lock = PJ_FALSE;
	}
}
MLIB_LOCAL void mlib_module_close() {

	mlib_module_t *point = clist.next;
	mlib_module_t *end = (mlib_module_t*) &clist;
	mlib_module_t *next;
	init = PJ_FALSE;
	// unload all mod
	while (point != end) {
		next = point->next;
		mlib_module_unload(point);
		point = next;
	}
	while (clist.next != end) {
		pj_thread_sleep(10);
	}
#if MLIB_MINI_SYSTEM == 1
	pj_lock_destroy(module_lock);
	mlib_pool_release(mpool);
#endif
}
mlib_module_t* mlib_module_load(const pj_str_t *name, const pj_str_t *path) {
	const char *lpath;
	if (path == NULL) {
		if (conf == NULL) {
			PJ_LOG(5, (MLIB_NAME, "module config is not setting"));
			return NULL;
		}
		pj_str_t mod = pj_str("module_path");
		const pj_str_t *tst = NULL;
		mlib_context_val *val = mlib_context_type_get_value(conf, &mod);
		if (val)
			tst = mlib_context_val_to_str(val);
		if (tst == NULL) {
			PJ_LOG(4, (MLIB_NAME , "module path is not declear"));
			return NULL;
		}
		lpath = tst->ptr;
	} else {
		lpath = path->ptr;
	}
	char buff[300];
	sprintf(buff, "%s%c%.*s", lpath, MLIB_PATH_SP, _ptr(name));
	void *handle = dlopen(buff, RTLD_LAZY);
	if (handle == NULL) {
		PJ_LOG(5,
				(MLIB_NAME, "mod name %.*s is not exit \n on mod path %s", _ptr(name), lpath));
		return PJ_FALSE;
	}
	mlib_module_object *amo = dlsym(handle, "module_mod");
	if (amo == NULL) {
		PJ_LOG(5,
				(MLIB_NAME, "the mod %.*s is not have module_mod object ", _ptr(name)));
		return PJ_FALSE;
	}

	mlib_module_t *reb = mlib_module_simple(name);
	pj_pool_t *pool = reb->pool;
	reb->type = module_type_dynamic_load;
	reb->dynamic.handle = handle;
	reb->dynamic.mod = amo;
	if (reb->dynamic.mod->load(reb, &reb->dynamic.mod_data) != PJ_SUCCESS) {
		dlclose(handle);
		pj_list_erase(reb);
		mlib_pool_release(pool);
		return NULL;
	}
	return reb;
}
void mlib_module_unload(mlib_module_t *mod) {
	pj_bool_t clear_lock;
	if (mod->name.slen > 0)
		PJ_LOG(6,
				(MLIB_NAME,"Release module for %.*s", mod->name.slen , mod->name.ptr));
	_mlock(mod);
	clear_lock = !mod->clear_lock;
	mod->clear_lock = PJ_TRUE;
	struct mlib_module_ctl *first = mod->list.next;
	struct mlib_module_ctl *end = (struct mlib_module_ctl*) &mod->list;
	while (first != end) {
		first = first->next;
		ctl_release(first->prev);
	}

	_mrls(mod);
	if (clear_lock) {
		clear_mod(mod);
	}
}

mlib_module_t* mlib_module_simple(const pj_str_t *name) {
	pj_pool_t *pool = mlib_pool_create(NULL, MLIB_POOL_SIZE, MLIB_POOL_SIZE);
	mlib_module_t *amc = pj_pool_zalloc(pool, sizeof(mlib_module_t));
	amc->type = module_type_simple;
	amc->clear_lock = PJ_FALSE;
	amc->pool = pool;
	if (name) {
		pj_strdup(pool, &amc->name, name);
	}
	pj_list_init(&amc->list);
	pj_list_init(&amc->rls);

#if MLIB_MINI_SYSTEM !=1
	pj_lock_create_recursive_mutex(pool, name, &amc->lock);
#endif
	pj_list_insert_after(&clist, amc);
	return amc;
}
pj_pool_t* mlib_module_pool(mlib_module_t *mod) {
	return mod->pool;
}
void mlib_module_add_callback(mlib_module_t *mod, void *data,
		mlib_clear_callback clear) {
	struct mod_release *mrls = pj_pool_alloc(mod->pool,
			sizeof(struct mod_release));
	mrls->clear = clear;
	mrls->data = data;
	_mlock(mod)
		pj_list_insert_after(&mod->rls, mrls);
	_mrls(mod)
}
void mlib_module_conf(const mlib_context_l *ctx) {

}
/* mod controler */
#define get_mod(p) (struct mlib_module_ctl *)((void*)p - sizeof(struct mlib_module_ctl))

//static void ctl_release(struct mlib_module_ctl *ctl) {

static void ctl_release(struct mlib_module_ctl *ctl) {
	PJ_LOG(6, (MLIB_FUNC,"release mod"));
	pj_bool_t clear_lock;
	mlib_module_t *mod = ctl->mctl;
	_mlock(ctl->mctl);
	clear_lock = !ctl->mctl->clear_lock;
	ctl->mctl->clear_lock = PJ_TRUE;
	pj_list_erase(ctl);
	if (ctl->un_reg)
		ctl->un_reg(ctl + 1);
	_mrls(ctl->mctl)
#if MLIB_MODULE_TMP_POOL
	if (ctl->pool)
		mlib_pool_release(ctl->pool);
#endif
	if (clear_lock) {
		clear_mod(mod);
	}
}

pj_pool_t* mlib_modctl_pool(void *pointer) {
	struct mlib_module_ctl *sp = get_mod(pointer);
	return sp->pool;
}
static void rls_modctl(void *arg) {
	struct mlib_module_ctl *ctl = get_mod(arg);
	ctl_release(ctl);
}
void* mlib_modctl_alloc(mlib_module_t *mctl, int size,
		mlib_clear_callback un_reg) {
	if (size == 0) {
		size = 4;
		PJ_LOG(4, (MLIB_FUNC,"add more than 4 byte to avoid conflit"));
	}
	pj_pool_t *pool;
#if MLIB_MODULE_TMP_POOL
	pool = mlib_pool_create("mpool", MLIB_POOL_SIZE, MLIB_POOL_SIZE);
#else
	pool = mctl->pool;
#endif
	struct mlib_module_ctl *mod;
	int z = sizeof(struct mlib_module_ctl);
	void *res = pj_pool_zalloc(pool, z + size);
	mod = res;
	pj_list_init(mod);
	mod->pool = pool;
	mod->mctl = mctl;
	mod->un_reg = un_reg;
	mlib_mem_bind(pool, mod + 1, rls_modctl);
	_mlock(mctl)
		pj_list_insert_nodes_after(&mctl->list, mod);
	_mrls(mctl)
	return res + z;
	// return mod + 1 ;
}

void mlib_modctl_release(void *pointer) {
	mlib_mem_mask_destroy(pointer);
}
