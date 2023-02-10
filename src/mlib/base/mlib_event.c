/*
 * event.c
 *
 *  Created on: Aug 20, 2022
 *      Author: amneiht
 */

#include <mlib/event.h>
#include <mlib/mem.h>
#include <mlib_local.h>

#define CORE_START
#define CORE_END

static pj_bool_t need_init = PJ_TRUE;
struct mlib_event_handle_t {
	PJ_DECL_LIST_MEMBER(struct mlib_event_handle_t )
	;
	unsigned int id;
	mlib_handle_p handle;
	mlib_event_t *parrent;
	void *data;
	mlib_clear_callback clear;
};
struct mlib_event_t {
	PJ_DECL_LIST_MEMBER(struct mlib_event_t)
	;
	pj_str_t name;
	pj_lock_t *lock;
	pj_bool_t is_destroy;
	pj_list list;
	unsigned int id;
};

static pj_list hlist;
static int find_id(void *p, const void *note) {
	int *id = (int*) p;
	const mlib_event_t *m = note;
	if (m->id == *id) {
		return mlib_search_found;
	}
	return mlib_search_notfound;
}
struct mlib_event_t* get_id(int id) {
	return pj_list_search(&hlist, &id, find_id);
}
static void list_destroy(pj_list *list) {
	mlib_mem_release_list(list);
}
static void _mlib_event_init() {
// event destroy ler
	pj_list_init(&hlist);
	need_init = PJ_FALSE;
}
static void clear_evt(void *arg) {
	mlib_event_t *evt = arg;
	PJ_LOG(3,
			(MLIB_NAME, "destroy event name %.*s", (int) evt->name.slen, evt->name.ptr));
	evt->is_destroy = PJ_TRUE;
	pj_lock_acquire(evt->lock);
	{
		// release all event handle
		list_destroy(&evt->list);
	}
	pj_lock_release(evt->lock);
	pj_lock_destroy(evt->lock);
	pj_list_erase(evt);
}
mlib_event_t* mlib_event_create(mlib_module_t *mod, const char *event_name) {
	if (need_init)
		_mlib_event_init();

	PJ_LOG(6, (ACORE_NAME, "create event with name %s", event_name));

//	mlib_event_t *evt = mlib_list_ele_create(pool, sizeof(mlib_event_t));
	mlib_event_t *evt = mlib_modctl_alloc(mod, sizeof(mlib_event_t), clear_evt);
	pj_pool_t *pool = mlib_modctl_pool(evt);
	pj_lock_create_recursive_mutex(pool, event_name, &evt->lock);
	pj_list_init(&evt->list);
	pj_strdup2(pool, &evt->name, event_name);
	evt->is_destroy = PJ_FALSE;
	evt->id = pj_rand();
	CORE_START
	pj_list_insert_before(&hlist, evt);
	CORE_END
	return evt;
}

// call close funtion all event handle
pj_status_t mlib_event_destroy(mlib_event_t *evt) {
	PJ_LOG(5,
			(MLIB_NAME, "destroy event name %.*s", (int) evt->name.slen, evt->name.ptr));
	CORE_START
	mlib_mem_mask_destroy(evt);
	CORE_END
	return PJ_SUCCESS;
}
static void event_handle_clear(void *arg) {
	mlib_event_handle_t *handle = arg;
	mlib_event_t *evt = handle->parrent;
	if (evt == NULL || evt->is_destroy) {
		PJ_LOG(5,
				(MLIB_FUNC, "Can Not remove handle to event %.*s", (int) evt->name.slen, evt->name.ptr));
	}

	pj_list_erase(handle);
	if (handle->clear)
		handle->clear(handle->data);
}

void mlib_event_register(mlib_event_t *evt, mlib_event_handle_t *handle) {
	pj_lock_acquire(evt->lock);
	{
		pj_list_insert_before(&evt->list, handle);
	}
	handle->parrent = evt;
	pj_lock_release(evt->lock);
}

mlib_event_handle_t* mlib_event_handle_create(mlib_module_t *mod,
		mlib_handle_p handle, mlib_clear_callback clear, void *data) {

	mlib_event_handle_t *po = mlib_modctl_alloc(mod,
			sizeof(mlib_event_handle_t), event_handle_clear);
	pj_list_init(po);
	po->id = mlib_rand();
	po->clear = clear;
	po->data = data;
	po->handle = handle;
	return po;
}
void mlib_event_unregister_handle(mlib_event_handle_t *handle) {
	mlib_modctl_release(handle);
}
struct evt_data {

	int type;
	void *data;
};
void mlib_event_send(const mlib_event_t *evt, int type, void *data) {
	if (evt == NULL || evt->is_destroy) {
		PJ_LOG(5,
				(MLIB_FUNC, "Remove handle to event %.*s", (int) evt->name.slen, evt->name.ptr));
		return;
	}
	pj_lock_acquire(evt->lock);
//	struct evt_data em = { .type = type, .data = data };
//	pj_list_search((void*) &evt->list, &em, send_event);
	mlib_event_handle_t *first, *last, *point;
	first = (mlib_event_handle_t*) evt->list.next;
	last = (mlib_event_handle_t*) &evt->list;
	while (last != first) {
		point = first->next;
		first->handle(first->data, type, data);
		first = point;
	}
	pj_lock_release(evt->lock);
}

