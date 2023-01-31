/*
 * inv.c
 *
 *  Created on: Jan 30, 2023
 *      Author: amneiht
 */
#include <pjlib.h>
#include <msip/ua.h>

struct msip_call {
	PJ_DECL_LIST_MEMBER(struct msip_call)
	;
	msip_ua *ua;
	pj_str_t media;
	pj_bool_t stop;
	pjsip_inv_session *inv;
	int state;
	call_module *mod;
};
