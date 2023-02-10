/*
 * inv.c
 *
 *  Created on: Jan 30, 2023
 *      Author: amneiht
 */
#include <pjlib.h>
#include <msip/ua.h>
#include "msip_local.h"
#include <mlib/mem.h>
#include <msip/event.h>
#include <msip/base.h>

static pj_bool_t cb_on_invite(pjsip_rx_data *rdata);
static void call_on_tsx_state_changed(pjsip_inv_session *inv,
		pjsip_transaction *tsx, pjsip_event *e);

extern MLIB_LOCAL msip_media_t* _msip_media_sdp(const pjmedia_sdp_session *sdp);
extern MLIB_LOCAL msip_media_t* _msip_media_name(const pj_str_t *name);

#define BEGIN pj_lock_acquire(inv_lock) ;
#define END  pj_lock_release(inv_lock) ;

static pj_lock_t *inv_lock;
static mlib_mmap_t *inv_map;
struct inv_object {
	pjsip_rx_data *rdata;
	pjsip_inv_session *inv;
	pj_str_t caller;
	pjmedia_sdp_session *sdp;
};
typedef enum app_call_state {
	app_call_state_null,
	app_call_state_wait,
	app_call_state_incall,
	app_call_state_handle,
	app_call_state_complete,
} app_call_state;

static pjsip_module invite_module = {
NULL, NULL, /* prev, next.		*/
{ "msip_invite_module", 18 }, /* Name.			*/
-1, /* Id			*/
PJSIP_MOD_PRIORITY_APPLICATION - 1, /* Priority			*/
NULL, /* load()			*/
NULL, /* start()			*/
NULL, /* stop()			*/
NULL, /* unload()			*/
&cb_on_invite, /* on_rx_request()		*/
NULL, /* on_rx_response()		*/
NULL, /* on_tx_request.		*/
NULL, /* on_tx_response()		*/
NULL, /* on_tsx_state()		*/
};
static msip_call* msip_call_find_by_inv(pjsip_inv_session *inv) {
	return mlib_mmap_get(inv_map, inv);
}
static void clear_call(void *arg) {
	pj_list_erase(arg);
	msip_call *call = arg;
	mlib_mem_dec_ref(call->mod);
	mlib_mem_dec_ref(call->ua);
	mlib_pool_release(call->pool);
	PJ_LOG(4, ("inv","clear call "));
}
static msip_call* create_call(msip_media_t *obj, msip_ua *ua) {
	pj_pool_t *pool = mlib_pool_create(NULL, 1024, 1024);
	msip_call *acall = mlib_mem_alloc(pool, sizeof(msip_call), clear_call);
	acall->mod = obj;
	acall->pool = pool;
	acall->ua = ua;
	pj_list_insert_after(&ua->call, acall);
//	mlib_mem_bind(pool, acall, clear_call);
	mlib_mem_add_ref(ua);
	mlib_mem_add_ref(obj);
	return acall;
}

static pj_bool_t cb_on_invite(pjsip_rx_data *rdata) {
	pj_status_t status;
	pjmedia_sdp_session *sdp;
	pjsip_dialog *dlg;
	pjsip_inv_session *inv;
	pjsip_tx_data *tdata;
	pj_str_t caller = { NULL, 0 };
	pj_str_t callee = { NULL, 0 };
	msip_call *acall = NULL;
	local_str(contact, 300);

	if (rdata->msg_info.msg->line.req.method.id != pjsip_invite_method.id)
		return PJ_FALSE;
// find ua
	msip_ua *ua = msip_ua_find_by_uri(rdata->msg_info.to->uri);
	if (ua == NULL) {
		PJ_LOG(3, ("inv","No matching ua"));
		return PJ_FALSE;
	}
// check control
	pjsip_uri *uri = rdata->msg_info.from->uri;
	if (PJSIP_URI_SCHEME_IS_SIP(uri) || PJSIP_URI_SCHEME_IS_SIPS(uri)) {
		pjsip_sip_uri *suri = (pjsip_sip_uri*) pjsip_uri_get_uri(uri);
		caller = suri->user;
	}
	callee = ua->login.cred.username;
// print contact
	msip_ua_print_contact(ua, contact);
// create inv session
	status = pjmedia_sdp_parse(rdata->tp_info.pool,
			rdata->msg_info.msg->body->data, rdata->msg_info.msg->body->len,
			&sdp);
	status = pjsip_dlg_create_uas_and_inc_lock(pjsip_ua_instance(), rdata,
			contact, /* contact */
			&dlg);

	status |= pjsip_inv_create_uas(dlg, rdata, NULL, 0, &inv);

	if (status != PJ_SUCCESS) {
		PJ_LOG(5, (MLIB_NAME,"cannot create inv"));
		pjsip_dlg_create_response(dlg, rdata, 400, NULL, &tdata);
		pjsip_dlg_send_response(dlg, pjsip_rdata_get_tsx(rdata), tdata);
		pjsip_dlg_dec_lock(dlg);
		return PJ_TRUE;
	}
	mlib_container cont;
	pj_bool_t active = PJ_TRUE;
	pj_bzero(&cont, sizeof(cont));
	cont.data[pos_ua] = ua;
	cont.data[pos_callee] = &callee;
	cont.data[pos_caller] = &caller;
	cont.data[pos_remote_sdp] = sdp;
	cont.data[pos_active] = &active;
	cont.data[pos_inv] = inv;
	msip_media_t *callm = _msip_media_sdp(sdp);
	if (callm == NULL) {
		pj_str_t respone = pj_str("No Support media");
		pjsip_tx_data *tdata;
		if (inv != NULL) {
			pjsip_inv_end_session(inv, 488, &respone, &tdata);
			pjsip_inv_send_msg(inv, tdata);
		}
		mlib_event_send(_msip_obj->event, MSIP_EVENT_INV_STATE_START_FALSE,
		NULL);
	}
	cont.data[pos_media] = callm;
	mlib_event_send(_msip_obj->event, MSIP_EVENT_INV_STATE_INCOMING, &cont);

	if (!active) {
		PJ_LOG(3, (MLIB_NAME,"call is reject bay event handle"));
		pj_str_t res = pj_str("Call Reject");
		pjsip_inv_end_session(inv, 400, &res, &tdata);
		pjsip_inv_send_msg(inv, tdata);
		return PJ_TRUE;
	}

	BEGIN
// create get media

	if (callm == NULL) {
		pj_str_t respone = pj_str("No Support media");
		pjsip_tx_data *tdata;
		if (inv != NULL) {
			pjsip_inv_end_session(inv, 488, &respone, &tdata);
			pjsip_inv_send_msg(inv, tdata);
		}
	} else {
		// init 180 Ringg Sip Method
		status = pjsip_inv_initial_answer(inv, rdata, 180,
		NULL, NULL, &tdata);
		pjsip_inv_send_msg(inv, tdata);
		// call control
		acall = create_call(callm, ua);
		pj_pool_t *pool = acall->pool;
		pj_strdup(pool, &acall->caller, &caller);
		acall->callee = ua->login.cred.username;
		acall->state = app_call_state_wait;
		acall->inv = inv;
		mlib_mmap_set(inv->pool, inv_map, inv, acall);
		pj_list_insert_after(&ua->call, acall);
		// callback for call module
		local_str(pika, 200);
		pjmedia_sdp_session *lsdp = callm->media.accept_sdp(acall, sdp,
				callm->media.data, pika);
		if (lsdp == NULL) {
			pjsip_tx_data *tdata;
			if (inv != NULL) {
				pjsip_inv_end_session(inv, 488, pika, &tdata);
				pjsip_inv_send_msg(inv, tdata);
			}
			goto CLOSE;
		}
		pjsip_inv_set_local_sdp(inv, lsdp);
		if (ua->auto_accept) {
			acall->state = app_call_state_wait;
			msip_call_accept(acall);
		} else {
			acall->state = app_call_state_wait;
			callm->media.call_iscoming(acall, callm->media.data);
		}
	}

	CLOSE:
	END
	return PJ_TRUE;
}
static void call_on_media_update(pjsip_inv_session *inv, pj_status_t status) {
	if (!inv)
		return;
	msip_call *call = msip_call_find_by_inv(inv);
	if (call == NULL) {
		PJ_LOG(3, ("on_media_update" , "can't not find msip_call"));
		return;
	}
	BEGIN
	if (call->state < app_call_state_handle) {
		mlib_container cont;

		pj_bzero(&cont, sizeof(cont));
		cont.data[pos_ua] = call->ua;
		cont.data[pos_callee] = &call->callee;
		cont.data[pos_caller] = &call->caller;
		cont.data[pos_inv] = inv;
		cont.data[pos_call] = call;
		mlib_event_send(_msip_obj->event, MSIP_EVENT_INV_STATE_MEDIA, &cont);
		call->state = app_call_state_incall;
		call->mod->media.on_media_update(call, call->mod->media.data);
	}
// end state change
	END
}

static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e) {
	(void) inv;
	(void) e;
}
static void call_on_state_changed(pjsip_inv_session *inv, pjsip_event *e) {
	msip_call *call = msip_call_find_by_inv(inv);
	if (call == NULL) {
		PJ_LOG(5, (MLIB_NAME , "cant not find msip_call"));
		return;
	}
	BEGIN
	call->mod->media.on_state_changed(call, inv->state, call->mod->media.data);

	if (inv->state == PJSIP_INV_STATE_DISCONNECTED) {
		call->state = app_call_state_complete;
		mlib_container cont;
		pj_bzero(&cont, sizeof(cont));
		cont.data[pos_ua] = call->ua;
		cont.data[pos_callee] = &call->callee;
		cont.data[pos_caller] = &call->caller;
		cont.data[pos_inv] = inv;
		cont.data[pos_call] = call;

		mlib_event_send(_msip_obj->event, MSIP_EVENT_INV_STATE_DISCONNECTED,
				&cont);
		msip_ua *ua = call->ua;

		mlib_mem_mask_destroy(call);
		mlib_mmap_unset(inv_map, inv);
	}

	END
}
static void call_on_tsx_state_changed(pjsip_inv_session *inv,
		pjsip_transaction *tsx, pjsip_event *e) {
	msip_call *call = msip_call_find_by_inv(inv);
	if (call == NULL) {
		PJ_LOG(3, ("on_tsx_state" , "cant not find msip_call"));
		return;
	}
	BEGIN
	if (tsx->state == PJSIP_TSX_STATE_COMPLETED
			&& call->state < app_call_state_handle) {
		call->state = app_call_state_handle;
	}
	END
}
static void clear_inv(void *arg) {
	pj_lock_t *lock = arg;
	pj_lock_destroy(lock);
}
MLIB_LOCAL void _msip_inv_module() {
	pjsip_endpoint *ep = msip_endpt();
	pjsip_inv_callback inv_cb;
	pjsip_endpt_register_module(ep, &invite_module);
	pj_bzero(&inv_cb, sizeof(inv_cb));
	inv_cb.on_state_changed = call_on_state_changed;
	inv_cb.on_new_session = call_on_forked;
	inv_cb.on_media_update = call_on_media_update;
	inv_cb.on_tsx_state_changed = call_on_tsx_state_changed;
	/* Initialize invite session module:  */
	pj_status_t st = pjsip_inv_usage_init(ep, &inv_cb);
	if (st != PJ_SUCCESS) {
		PJ_LOG(1, (MLIB_FUNC, "loi set sup inv call back\n"));
	}

	inv_map = mlib_mmap_create(mlib_module_pool(msip_mod()));
	pj_lock_create_recursive_mutex(mlib_module_pool(msip_mod()), "inv lock",
			&inv_lock);
	mlib_module_add_callback(msip_mod(), inv_lock, clear_inv);
}
/*  user call */
msip_call* msip_ua_call(msip_ua *ua, const pj_str_t *callee,
		const char *media_type, void *data) {
	if (ua->login.state != msip_state_login)
		return NULL;
	pj_status_t status;
	pjsip_dialog *dlg;
	pjsip_inv_session *inv;
	pjsip_tx_data *tdata;
	pj_bool_t active = PJ_TRUE;
	msip_cvstr(type, media_type);
	msip_call *scall = NULL;
	mlib_container *conf = mlib_alloca(mlib_container);
	pj_bzero(conf, sizeof(mlib_container));
	conf->data[pos_ua] = ua;
	conf->data[pos_callee] = &callee;
	conf->data[pos_caller] = &ua->login.cred.username;
	conf->data[pos_active] = &active;
	msip_media_t *call_mod = _msip_media_name(type);
	if (!call_mod) {
		mlib_event_send(_msip_obj->event, MSIP_EVENT_INV_STATE_START_FALSE,
				conf);
		return NULL;
	}
	mlib_event_send(_msip_obj->event, MSIP_EVENT_INV_STATE_START, conf);
	if (!active) {
		PJ_LOG(3, (MLIB_FUNC,"call reject bay envent handle"));
		return NULL;
	}

	BEGIN
	local_str(res, 300);
	local_str(remote_uri, 300);
	local_str(local_uri, 300);
	local_str(contact, 300);
	msip_ua_print_call_uri(ua, callee, remote_uri);
	msip_ua_print_uri(ua, local_uri);
	msip_ua_print_contact(ua, contact);
// create dialog
	status = pjsip_dlg_create_uac(pjsip_ua_instance(), local_uri, /* local URI */
	contact, /* local Contact */
	remote_uri, /* remote URI */
	remote_uri, /* remote target */
	&dlg); /* dialog */

	status = pjsip_inv_create_uac(dlg, NULL, 0, &inv);
	if (status != PJ_SUCCESS) {
		PJ_LOG(2, (MLIB_FUNC,"Can't not create inv"));
		pjsip_dlg_terminate(dlg);
		// tmp call
		mlib_event_send(_msip_obj->event, MSIP_EVENT_INV_STATE_START_FALSE,
				conf);
		goto CLOSE;
	}
	status = pjsip_auth_clt_set_credentials(&dlg->auth_sess, 1,
			&ua->login.cred);
	if (status != PJ_SUCCESS) {
		PJ_LOG(2, (MLIB_FUNC,"Can't not create inv"));
		pjsip_dlg_terminate(dlg);
		// tmp call
		pjsip_inv_terminate(inv, 400, 0);
		mlib_event_send(_msip_obj->event, MSIP_EVENT_INV_STATE_START_FALSE,
				conf);
		goto CLOSE;
	}
// create call add to bind list
	scall = create_call(call_mod, ua);
	scall->inv = inv;
	scall->caller = ua->login.cred.username;
	pj_strdup(inv->pool, &scall->callee, callee);
	pj_list_insert_after(&ua->call, scall);

	pjmedia_sdp_session *lsdp = call_mod->media.inv_sdp(scall,
			scall->mod->media.data, res);
	pjsip_inv_set_local_sdp(inv, lsdp);
	mlib_mmap_set(inv->pool, inv_map, inv, scall);
// modyfine invite session
	status = pjsip_inv_invite(inv, &tdata);
	status = pjsip_inv_send_msg(inv, tdata);
	scall->state = app_call_state_wait;
	CLOSE:
	END
	return scall;

}
void msip_call_accept(msip_call *call) {
	BEGIN
	if (mlib_mem_check(call)) {
		pjsip_tx_data *tdata;
		if (call->state == app_call_state_wait) {
			pjsip_inv_answer(call->inv, 200, NULL, NULL, &tdata);
			pjsip_inv_send_msg(call->inv, tdata);
		}
	}
	END
}
void msip_call_reject(msip_call *call) {
	BEGIN
	if (mlib_mem_check(call)) {
		pj_str_t respone = pj_str("REJECT");
		pjsip_tx_data *tdata;
		if (call->inv != NULL) {
			pjsip_inv_end_session(call->inv, 400, &respone, &tdata);
			pjsip_inv_send_msg(call->inv, tdata);
		}
	}
	END
}
void msip_call_close(msip_call *call) {
	BEGIN
	if (mlib_mem_check(call)) {
		pjsip_tx_data *tdata;
		if (call->inv != NULL) {
			pjsip_inv_end_session(call->inv, 200, NULL, &tdata);
			pjsip_inv_send_msg(call->inv, tdata);
		}
	}
	END
}
