/*
 * event.h
 *
 *  Created on: Jan 28, 2023
 *      Author: amneiht
 */

#ifndef MSIP_EVENT_H_
#define MSIP_EVENT_H_

#include <mlib/event.h>
#include <msip/ua.h>
enum MSIP_EVENT_INV_STATE {
	// register event
	MSIP_EVENT_REGISTERING = 0,
	MSIP_EVENT_REGISTER_OK,
	MSIP_EVENT_REGISTER_FAILSE,
	// Call event
	MSIP_EVENT_INV_STATE_START, // send event when call MSIP_ua_call()
	MSIP_EVENT_INV_STATE_START_FALSE, // send event when no match call and can not send invite
	MSIP_EVENT_INV_STATE_INCOMING, // send event when recieve SIP IVITE
	MSIP_EVENT_INV_STATE_MEDIA, // send event when sdp exchange OK
	MSIP_EVENT_INV_STATE_DISCONNECTED, // send event when call is close
	MSIP_EVENT_MAX
};

void msip_event_register(mlib_event_handle_t *handle);

msip_ua* msip_event_get_ua(const mlib_container *event_data);
const pjmedia_sdp_session* msip_event_get_remote_sdp(
		const mlib_container *event_data);
const pjmedia_sdp_session* msip_event_get_local_sdp(
		const mlib_container *event_data);
const pj_str_t* msip_event_get_caller(const mlib_container *event_data);
const pj_str_t* msip_event_get_callee(const mlib_container *event_data);

#endif /* MSIP_EVENT_H_ */
