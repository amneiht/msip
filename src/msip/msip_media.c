/*
 * msip_media.c
 *
 *  Created on: Jan 30, 2023
 *      Author: amneiht
 */

#include <msip/inv.h>
#include <mlib/mem.h>
#include "msip_local.h"
static pj_bool_t media_init = PJ_FALSE;
static pj_lock_t *mlock;
struct media_control {
	pj_list media_list;
};
static struct media_control *mctl = NULL;

static void clear_mod(void *arg) {
	struct media_control *ctl = arg;
	mlib_mem_release_list(&ctl->media_list);
	media_init = PJ_FALSE;
}
static void clear_lock(void *arg) {
	pj_lock_destroy(mlock);
}
extern void MLIB_LOCAL _mlib_media_init() {

	if (_msip_obj == NULL || media_init)
		return;
	media_init = PJ_TRUE;
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
static int ele_cmp(const void *ele1, const void *ele2) {
	const msip_media_t *call_1 = ele1;
	const msip_media_t *call_2 = ele2;
	return (int) (call_1->media.pri - call_2->media.pri);
}
msip_media_t* msip_media_register(mlib_module_t *mod, const msip_media_obj *obj,
		mlib_clear_callback clear) {

	_mlib_media_init();
	msip_media_t *m_mod = mlib_modctl_alloc(mod, sizeof(struct msip_media_t),
			clear_media);
	pj_pool_t *pool = mlib_modctl_pool(m_mod);
	pj_memcpy(&m_mod->media, obj, sizeof(msip_media_obj));
	pj_strdup(pool, &m_mod->media.module_name, &obj->module_name);
	m_mod->clear = clear;
	pj_lock_acquire(mlock);
	msip_list_add(&mctl->media_list, m_mod, ele_cmp);
	pj_lock_release(mlock);
	return m_mod;
}
void msip_media_unregier(msip_media_t *ms_media) {
	mlib_mem_mask_destroy(ms_media);
}
static int find_med(void *value, const void *node) {
	const msip_media_t *med = node;
	const pj_str_t *name = value;
	if (!mlib_mem_check(node)) {
		return mlib_search_notfound;
	}
	return !med->media.match_media(name);
}
static int find_sdp(void *value, const void *node) {
	const msip_media_t *med = node;
	const pjmedia_sdp_session *sdp = value;
	if (!mlib_mem_check(node)) {
		return mlib_search_notfound;
	}
	return !med->media.match_sdp(sdp);
}
msip_ua* msip_call_ua(msip_call *call) {
	return call->ua;
}
pjsip_inv_session* msip_call_inv(msip_call *call) {
	return call->inv;
}
const pj_str_t* msip_call_get_caller(msip_call *call) {
	return &call->caller;
}
const pj_str_t* msip_call_get_callee(msip_call *call) {
	return &call->callee;
}

MLIB_LOCAL msip_media_t* _msip_media_name(const pj_str_t *name) {
	if (!media_init)
		return NULL;
	msip_media_t *res = NULL;
	pj_lock_acquire(mlock);
	res = pj_list_search(&mctl->media_list, (void*) name, find_med);
	pj_lock_release(mlock);
	return res;

}
MLIB_LOCAL msip_media_t* _msip_media_sdp(const pjmedia_sdp_session *sdp) {
	if (!media_init)
		return NULL;
	msip_media_t *res = NULL;
	pj_lock_acquire(mlock);
	res = pj_list_search(&mctl->media_list, (void*) sdp, find_sdp);
	pj_lock_release(mlock);
	return res;

}
