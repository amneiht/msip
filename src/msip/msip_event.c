/*
 * event.c
 *
 *  Created on: Jan 28, 2023
 *      Author: amneiht
 */

#include <msip/event.h>
#include "msip_local.h"

void msip_event_register(mlib_event_handle_t *handle) {
	mlib_event_register(_msip_obj->event, handle);
}
MLIB_LOCAL void _msip_event_send(int id, mlib_container *con) {
	mlib_event_send(_msip_obj->event, id, con);
}

msip_ua* msip_event_get_ua(mlib_container *event_data) {
	return (msip_ua*) event_data->data[pos_ua];
}
pjmedia_sdp_session* msip_event_get_remote_sdp(mlib_container *event_data) {
	return (pjmedia_sdp_session*) event_data->data[pos_remote_sdp];
}
msip_ua* msip_event_get_local_sdp(mlib_container *event_data) {
	return (pjmedia_sdp_session*) event_data->data[pos_local_sdp];
}
const pj_str_t* msip_event_get_caller(mlib_container *event_data) {
	return (pj_str_t*) event_data->data[pos_caller];
}
const pj_str_t* msip_event_get_callee(mlib_container *event_data) {
	return (pj_str_t*) event_data->data[pos_callee];
}
