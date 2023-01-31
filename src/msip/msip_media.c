/*
 * msip_media.c
 *
 *  Created on: Jan 30, 2023
 *      Author: amneiht
 */

#include <msip/inv.h>
#include "msip_local.h"
static pj_bool_t media_init = PJ_FALSE;
static pj_lock_t *mlock;
struct media_control {
	pj_list media_list;
};
static struct media_control *mctl = NULL;

static void clear_mod(void *arg) {
	struct media_control *ctl = arg;
	mlib_modctl_list_destroy(&ctl->media_list);
	media_init = PJ_FALSE;
}
static void clear_lock(void *arg) {
	pj_lock_destroy(mlock);
}
extern void MLIB_LOCAL _mlib_media_init() {

	if (_msip_obj == NULL || media_init)
		return;
	media_init = PJ_FALSE;
	mctl = mlib_modctl_alloc(_msip_obj->mod, sizeof(struct media_control),
			clear_mod);
	pj_list_init(&mctl->media_list);
	pj_lock_create_recursive_mutex(mlib_module_pool(_msip_obj->mod),
			"media lock", &mlock);
	mlib_module_add_callback(_msip_obj->mod, NULL, clear_lock);
}
#define mcp(a,b,c) a->c = b->c

static void clear_media(void *data) {
	pj_lock_acquire(mlock);
	pj_list_erase(data);
	msip_media_t *m_mod = data;
	if (m_mod->clear)
		m_mod->clear(data);
	pj_lock_release(mlock);
}
msip_media_t* msip_media_register(mlib_module_t *mod, const msip_media_obj *obj,
		mlib_clear_callback clear) {

	msip_media_t *m_mod = mlib_modctl_alloc(mod, sizeof(struct msip_media_t),
			clear_media);
	pj_pool_t *pool = mlib_modctl_pool(m_mod);
	pj_strdup(pool, &m_mod->module_name, obj->module_name);
	mcp(m_mod, obj, on_start_call);
	mcp(m_mod, obj, on_call_incoming);
	mcp(m_mod, obj, match_media);
	mcp(m_mod, obj, match_sdp);
	mcp(m_mod, obj, data);
	m_mod->clear = clear;
	pj_lock_acquire(mlock);
	pj_list_insert_after(&mctl->media_list, m_mod);
	pj_lock_release(mlock);
	return m_mod;
}
void msip_media_unregier(msip_media_t *ms_media) {
	mlib_mem_mask_destroy(ms_media);
}
static int find_med(void *value, const void *node) {
	const msip_media_t *med = node;
	pj_str_t *name = value;
	if (!mlib_mem_is_available(node)) {
		return mlib_search_notfound;
	}
	return pj_strcmp(&med->module_name, name);
}
MLIB_LOCAL msip_media_t* _msip_media_find(pj_str_t *name) {
	if (!media_init)
		return NULL;
	msip_media_t *res = NULL;
	pj_lock_acquire(mlock);
	res = pj_list_search(&mctl->media_list, name, find_med);
	if (res)
		mlib_mem_add_ref(res);
	pj_lock_release(mlock);
	return res;

}
