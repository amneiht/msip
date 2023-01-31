/*
 * msip_local.h
 *
 *  Created on: Jan 17, 2023
 *      Author: amneiht
 */

#ifndef MSIP_MSIP_LOCAL_H_
#define MSIP_MSIP_LOCAL_H_

#include <pjsip.h>
#include <pjsip_ua.h>
#include <mlib/event.h>
#include <mlib/module.h>
#include <msip/ua.h>
#include <mlib/timer.h>

enum data_position {
	pos_ua = 0, // postion of ua
	pos_call, // postion of call
	pos_inv, // postion of invite
	pos_caller, //
	pos_callee, //
	pos_local_sdp, //postion of local_sdp
	pos_remote_sdp, // postion of remote_sdp
	pos_media // postion of media object
};

struct msip_system {
	mlib_module_t *mod;
	pjsip_endpoint *endpoint;
	struct {
		pjsip_tpfactory *tcp;
		pjsip_transport *udp;
	} transport;
	struct {
		mlib_timer_t *timer;
		pj_time_val poll_time;
	} time;
	mlib_event_t *event;
};

struct msip_ua {
	PJ_DECL_LIST_MEMBER(struct asip_ua)
	;
	pj_pool_t *pool;
	int port;
	pj_str_t transport;
	struct {
		pj_str_t uid;
		int localport;
		pj_str_t last_contact;
	} contact;
	struct {
		msip_ua_state state;
		pj_time_val tlogin;
		pjsip_cred_info cred;
		int login_time;
		int false_cont;
		pjsip_regc *regc;
	} login;
	struct {
		PJ_DECL_LIST_MEMBER(struct asip_call)
		;
	} call;

};
struct msip_media_t {
	PJ_DECL_LIST_MEMBER(struct msip_media_t)
	;
	pj_str_t module_name;
	int pri;
	pj_bool_t (*on_start_call)(msip_call *call, void *mod_data);
	pj_bool_t (*on_call_incoming)(msip_call *call,
			pjmedia_sdp_session *remote_sdp, void *user_data);
	pj_bool_t (*match_media)(const pj_str_t *media);
	pj_bool_t (*match_sdp)(const pjmedia_sdp_session *sdp);
	mlib_clear_callback clear;
	void *data;
};

extern MLIB_LOCAL struct msip_system *_msip_obj;
MLIB_LOCAL int msip_ua_transport_port(msip_ua *ua);

/* funtion declear */

MLIB_LOCAL void _msip_event_send(int id, mlib_container *con);

#endif /* MSIP_MSIP_LOCAL_H_ */
