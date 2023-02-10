/*
 * inv.h
 *
 *  Created on: Jan 30, 2023
 *      Author: amneiht
 */

#ifndef MSIP_INV_H_
#define MSIP_INV_H_

#include <mlib/module.h>
#include <msip/ua.h>
#include <pjmedia/sdp.h>
#include <pjsip-ua/sip_inv.h>

typedef struct msip_media_t msip_media_t;
typedef struct msip_call msip_call;

enum msip_media_priority {
	// highest priority level
	msip_priority_high = 0,
	// normal level
	msip_priority_normal = 4,
	// low level
	msip_priority_low = 16,
};

/*define pointer funtion */
typedef pj_bool_t (*msip_callback_media_update)(msip_call *call, void *mod_data);
typedef pj_bool_t (*msip_callback_state_changed)(msip_call *call,
		enum pjsip_inv_state state, void *mod_data);
typedef pjmedia_sdp_session* (*msip_callback_accept_sdp)(msip_call *call,
		pjmedia_sdp_session *remote_sdp, void *user_data, pj_str_t *res);
typedef pjmedia_sdp_session* (*msip_callback_inv_sdp)(msip_call *call,
		void *user_data, pj_str_t *res);
typedef void (*msip_callback_call_iscomming)(msip_call *call, void *user_data);

typedef struct msip_media_obj {
	pj_str_t module_name;
	int pri;
	msip_callback_call_iscomming call_iscoming;
	msip_callback_accept_sdp accept_sdp;
	msip_callback_inv_sdp inv_sdp;

	msip_callback_media_update on_media_update;
	msip_callback_state_changed on_state_changed;

	pj_bool_t (*match_media)(const pj_str_t *media);
	pj_bool_t (*match_sdp)(const pjmedia_sdp_session *sdp);
	void *data;
} msip_media_obj;

msip_media_t* msip_media_register(mlib_module_t *mod, const msip_media_obj *obj,
		mlib_clear_callback clear);
void msip_media_unregier(msip_media_t *ms_media);

msip_ua* msip_call_ua(msip_call *call);
pjsip_inv_session* msip_call_inv(msip_call *call);
const pj_str_t* msip_call_get_caller();
const pj_str_t* msip_call_get_callee();
/* call funtion */
msip_call* msip_ua_call(msip_ua *ua, const pj_str_t *callee,
		const char *media_type, void *data);
// call control
void msip_call_accept(msip_call *call);
void msip_call_reject(msip_call *call);
void msip_call_close(msip_call *call);

#endif /* MSIP_INV_H_ */
