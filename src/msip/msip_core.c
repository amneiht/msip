#include "msip_local.h"
#include <pjlib.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <mlib/module.h>
#include <msip/base.h>
#include <mlib/event.h>
#include <mlib_util/util.h>

MLIB_LOCAL struct msip_system *_msip_obj = NULL;
extern MLIB_LOCAL void _init_sip_ua();
extern void MLIB_LOCAL _mlib_media_init();

static pj_bool_t init = PJ_FALSE;

static void sip_time_loop(mlib_timer_t *entry, void *arg) {
	(void) entry;
	struct msip_system *sys = arg;
	pjsip_endpt_handle_events(sys->endpoint, &_msip_obj->time.poll_time);
}

static void clear_sip(void *arg) {
	(void) arg;
	pjsip_endpt_destroy(_msip_obj->endpoint);
	_msip_obj = NULL;
	init = PJ_FALSE;
}
static void util_event_handle(void *user_data, int type,
		mlib_container *event_data) {
	if (type == MLIB_CLOSE) {
		mlib_module_unload(_msip_obj->mod);
	}
}
pj_status_t msip_init() {
	if (init)
		return 1;

	pj_status_t status = mlib_init();
	pj_str_t name = pj_str("Msip");
	mlib_module_t *sip_mod = mlib_module_simple(&name);
	_msip_obj = pj_pool_zalloc(mlib_module_pool(sip_mod),
			sizeof(struct msip_system));
	_msip_obj->mod = sip_mod;
	pjsip_endpoint *endpt;
	status = pjsip_endpt_create(mlib_pool_factory(), "asip.V1", &endpt);
	status |= pjsip_tsx_layer_init_module(endpt);
	status |= pjsip_ua_init_module(endpt, NULL);
	status |= pjsip_100rel_init_module(endpt);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
	pj_uint16_t af = pj_AF_INET();
	pjsip_udp_transport_cfg udp_cfg;
	pjsip_udp_transport_cfg_default(&udp_cfg, af);
	if (PJ_AF_INET == af) {
		status = pjsip_udp_transport_start2(endpt, &udp_cfg,
				&_msip_obj->transport.udp);
	}
	//ipv6 is not suport for now

	//tcp transport
	if (PJ_AF_INET == af) {
		pjsip_tcp_transport_cfg tcp_cfg;
		pjsip_tcp_transport_cfg_default(&tcp_cfg, af);
		status = pjsip_tcp_transport_start3(endpt, &tcp_cfg,
				&_msip_obj->transport.tcp);
	}

	_msip_obj->endpoint = endpt;
	_msip_obj->event = mlib_event_create(sip_mod, "sip_event");
	// module register
	_init_sip_ua();
	_mlib_media_init();
	// handle for close
	mlib_event_handle_t *hand = mlib_event_handle_create(sip_mod,
			util_event_handle, NULL, NULL);
	mlib_util_event_add_handle(hand);
	// timer
	_msip_obj->time.timer = mlib_timer_entry_create(sip_mod, "sip_timer",
			PJ_TRUE, 500, sip_time_loop, _msip_obj);
	_msip_obj->time.poll_time.sec = 0;
	_msip_obj->time.poll_time.msec = 480;
	mlib_util_timer_register(_msip_obj->time.timer);
	mlib_module_add_callback(sip_mod, NULL, clear_sip);
	return 1;
}
void msip_close() {
	mlib_module_unload(_msip_obj->mod);
}
mlib_module_t* msip_mod() {
	return _msip_obj->mod;
}

pjsip_endpoint* msip_endpt() {
	return _msip_obj->endpoint;
}
