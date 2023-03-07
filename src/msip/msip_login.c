/*
 * login.c
 *
 *  Created on: Jan 30, 2023
 *      Author: amneiht
 */

#include <mlib/base.h>
#include <msip/ua.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <msip/event.h>
#include <msip/base.h>
#include "msip_local.h"

static void update_contact(struct pjsip_regc_tsx_cb_param *param) {
	msip_ua *ua;
	pj_str_t contact;
	if (param->cbparam.code == 401 || param->cbparam.code == 407) {
		ua = param->cbparam.token;
		if (ua == NULL) {
			PJ_LOG(4, (MLIB_FUNC, "ua not found"));
			return;
		}
		// check if contact is change by network change .. etc
		contact.ptr = alloca(500);
		msip_ua_print_contact(ua, &contact);
		if (pj_strcmp(&contact, &ua->contact.last_contact) != 0) {
			// update contact
			PJ_LOG(4, (MLIB_FUNC, "update contact"));
			pj_memcpy(ua->contact.last_contact.ptr, contact.ptr, contact.slen);
			ua->contact.last_contact.slen = contact.slen;
			param->contact[0] = ua->contact.last_contact;
			param->contact_cnt = 1;
		}

	}
}
static void reg_cb(struct pjsip_regc_cbparam *param) {
	PJ_LOG(3, (MLIB_NAME, "you has a regist code %d", param->code));
	int reg_code = param->code;
	msip_ua *ua = param->token;
	if (ua == NULL) {
		mlib_event_send(_msip_obj->event, MSIP_EVENT_REGISTER_FAILSE, NULL);
		return;
	}
	pj_gettimeofday(&ua->login.tlogin);
	if (param->code == 401) {
		ua->login.state = msip_state_error;
		ua->login.false_cont = 0;
	} else if (param->code >= 200 && param->code < 300) {
		ua->login.state = msip_state_login;
		ua->login.false_cont = 2;
	} else if (param->code == 408) {
		// 408 timout
		ua->login.false_cont--;
		ua->login.state = msip_state_disconnect;
		pjsip_regc_info rec_inf;
		pjsip_regc_get_info(ua->login.regc, &rec_inf);
		pjsip_transport *rec_tr = rec_inf.transport;
		if (rec_tr) {
			if (ua->login.false_cont < 0) {
				pjsip_transport_shutdown(rec_tr);
				rec_tr = NULL;
				PJ_LOG(4, (MLIB_NAME, "has transport"));
			}
		} else {
			pjsip_regc_release_transport(ua->login.regc);
			ua->login.state = msip_state_error;
			ua->login.tlogin.sec = 0;
			PJ_LOG(4, (MLIB_NAME, "null transport"));
		}
	} else {
		ua->login.false_cont = 0;
		ua->login.state = msip_state_disconnect;
	}
// send event
	if (reg_code / 100 == 2) {
		mlib_event_send(_msip_obj->event, MSIP_EVENT_REGISTER_OK, ua);
	} else {
		mlib_event_send(_msip_obj->event, MSIP_EVENT_REGISTER_FAILSE, ua);
	}
}
static int create_newreg(msip_ua *uas) {
	pj_status_t status;
	char buff[300], buff2[300];
	pj_str_t aor;
	pj_str_t url;
	status = pjsip_regc_create(msip_endpt(), uas, reg_cb, &uas->login.regc);
	if (status != PJ_SUCCESS) {
		uas->login.regc = NULL;
		return -19;
	}

	pjsip_regc_set_reg_tsx_cb(uas->login.regc, update_contact);
	aor.ptr = alloca(300);
	msip_ua_print_uri(uas, &aor);
	const pj_str_t *tps = msip_ua_get_transport(uas);
	sprintf(buff2, "%.*s;transport=%.*s", (int) aor.slen - 2, aor.ptr + 1,
			(int) tps->slen, tps->ptr);
	url = pj_str(buff2);
	msip_ua_print_contact(uas, &uas->contact.last_contact);
	status = pjsip_regc_init(uas->login.regc, &url, &aor, &aor, 1,
			&uas->contact.last_contact, uas->login.login_time);
	if (status != PJ_SUCCESS) {
		pj_str_t err = pjsip_strerror(status, buff, 200);
		PJ_LOG(2, (__FILE__, "init erer :%.*s", (int) err.slen, err.ptr));
		return -19;
	}
	pjsip_regc_set_delay_before_refresh(uas->login.regc, 5);
	status = pjsip_regc_set_credentials(uas->login.regc, 1, &uas->login.cred);

	// test for out bountproxy
//	pjsip_host_port hps;
//	hps.host = pj_str("192.168.1.204");
//	hps.port = 5060;
//	pjsip_regc_set_via_sent_by(uas->login.regc, &hps, NULL);

	if (status != PJ_SUCCESS) {
		fprintf(stderr, "loi cred \n");
		return -19;
	}
	if (status != PJ_SUCCESS) {
		char bs[100];
		pjsip_strerror(status, bs, 200);
		puts(bs);
	}
	return PJ_SUCCESS;
}
static void sip_connect(void *arg) {
	msip_ua *ua = arg;
	pjsip_tx_data *tdata;
	pj_status_t status = pjsip_regc_register(ua->login.regc, 1, &tdata);
	if (status != PJ_SUCCESS) {
		PJ_LOG(3, (MLIB_NAME, "pjsip_regc_register \n"));
		return;
	}
	status = pjsip_regc_send(ua->login.regc, tdata);
	if (status != PJ_SUCCESS) {
		PJ_LOG(3, (MLIB_NAME, "loi send register \n"));
		return;
	}

}
pj_bool_t msip_ua_register(msip_ua *ua) {
	if (ua->login.regc != NULL) {
		ua->login.state = msip_state_null;
		pjsip_regc_destroy(ua->login.regc);
	}
	create_newreg(ua);
	sip_connect(ua);
	return PJ_TRUE;
}
pj_bool_t msip_ua_unregister(msip_ua *ua) {
	if (ua->login.regc != NULL) {
		ua->login.state = msip_state_null;
		pjsip_regc_destroy(ua->login.regc);
	}
	ua->login.regc = NULL;
	return PJ_TRUE;
}

