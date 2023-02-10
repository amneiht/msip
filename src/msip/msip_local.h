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
#include <msip/inv.h>

#define local_str(name , len ) pj_str_t * name = mlib_alloca(pj_str_t); \
name->slen = len; \
name->ptr = alloca(len+4); \
name->ptr[len] = 0

#define msip_cvstr(a,b) pj_str_t* a = alloca(sizeof(pj_str_t)) ; \
a->slen = strlen(b); \
a->ptr = alloca(a->slen+1); \
a->ptr[a->slen]	= 0 ;\
pj_memcpy(a->ptr,b,a->slen)

/*  struct and data define  */
enum data_position {
	pos_ua = 0, // postion of ua
	pos_call, // postion of call
	pos_inv, // postion of invite
	pos_caller, //
	pos_callee, //
	pos_local_sdp, //postion of local_sdp
	pos_remote_sdp, // postion of remote_sdp
	pos_media, // postion of media object
	pos_active,
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
	PJ_DECL_LIST_MEMBER(struct msip_ua)
	;
	pj_pool_t *pool;
	int port;
	pj_str_t transport;
	pj_bool_t auto_accept;
	pj_lock_t *lock;
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
	pj_list call;
//	mlib_list(msip_call ,call )
};

struct msip_media_t {
	PJ_DECL_LIST_MEMBER(struct msip_media_t)
	;
	struct msip_media_obj media;
	mlib_clear_callback clear;
};

struct msip_call {
	PJ_DECL_LIST_MEMBER(struct msip_call)
	;
	msip_ua *ua;
	pj_pool_t *pool;
	pj_str_t caller;
	pj_str_t callee;
	pj_bool_t stop;
	pj_bool_t is_caller;
	pjsip_inv_session *inv;
	pj_time_val wait;
	int state;
	msip_media_t *mod;
};

/* local funtion */

extern MLIB_LOCAL struct msip_system *_msip_obj;
MLIB_LOCAL int msip_ua_transport_port(msip_ua *ua);

/* funtion declear */

MLIB_LOCAL void _msip_event_send(int id, mlib_container *con);

typedef pj_int32_t (*element_cmp)(const pj_list_type *ele1,
		const pj_list_type *ele2);

MLIB_LOCAL void msip_list_add(pj_list *a, pj_list_type *b, element_cmp ele_cmp);
#endif /* MSIP_MSIP_LOCAL_H_ */
