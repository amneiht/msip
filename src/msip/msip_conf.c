/*
 * msip_conf.c
 *
 *  Created on: Feb 14, 2023
 *      Author: amneiht
 */

#include <msip/base.h>
#include <mlib/context.h>
#include <mlib_util/util.h>
#include <msip/event.h>
#include "msip_local.h"
#include <pjsip.h>

#define lsip_str(a,b) pj_str_t a = pj_str(b);
#if PJSIP_HAS_TLS_TRANSPORT
static void tls_config(mlib_context_l *ctl) {
	pjsip_tls_setting set;
	pj_sockaddr local_addr;
	int af;
	// todo Add suport for ipv6 version
	af = pj_AF_INET();
	pjsip_tls_setting_default(&set);
	set.method = PJSIP_TLSV1_3_METHOD;
	pj_sockaddr_init(af, &local_addr, NULL, 0);
	// cert config

	mlib_context_val *ca, *crt, *key, *ver;
	const pj_str_t *ca_file, *crt_file, *key_file, *ver_str;

	mlib_context_t *tls_ct = mlib_context_list_find2(ctl, "tls");
	if (tls_ct) {
		crt = mlib_context_type_get_value2(tls_ct, "cert");
		if (!crt)
			goto TLS_SET;
		key = mlib_context_type_get_value2(tls_ct, "key");
		if (!key) {
			PJ_LOG(3, (MLIB_NAME,"key file must specify"));
			goto TLS_SET;
		}
		ca = mlib_context_type_get_value2(tls_ct, "ca_file");
		if (ca) {
			ca_file = mlib_context_val_to_str(ca);
			set.ca_list_file = *ca_file;
		} else {
			ca_file = NULL;
		}
		// setting tls
		crt_file = mlib_context_val_to_str(crt);
		key_file = mlib_context_val_to_str(key);
		set.cert_file = *crt_file;
		set.privkey_file = *key_file;

		ver = mlib_context_type_get_value2(tls_ct, "verify_server");
		if (ver) {
			ver_str = mlib_context_val_to_str(ver);
			if (pj_strcmp2(ver_str, "yes") == 0) {
				if (!ca) {
					PJ_LOG(1, (MLIB_NAME ,"verify sever required ca_file"));
				} else {
					set.verify_server = PJ_TRUE;
				}
			}
		}

	}
	TLS_SET: pjsip_tls_transport_start2(_msip_obj->endpoint, &set, &local_addr,
	NULL, 1, &_msip_obj->transport.tls);
}

#endif
struct sip_ctl {
	int max_call;
};

extern MLIB_LOCAL void msip_ua_retry();
static void sip_handle_p(void *user_data, int type, void *event_data) {
	struct sip_ctl *data = user_data;
	msip_ua *uas;
	switch (type) {
	case MSIP_EVENT_INV_STATE_START:
	case MSIP_EVENT_INV_STATE_INCOMING:
		uas = msip_event_get_ua((const mlib_container*) event_data);
		if (pj_list_size(&uas->call) >= data->max_call) {
			msip_event_disable_call(event_data);
		}
		break;
	default:
		break;
	}
}
static void timer_active_ua(mlib_timer_t *entry, void *data) {
	(void) entry;
	(void) data;
	PJ_LOG(5, (MLIB_NAME,"retry register ua"));
	msip_ua_retry();
}

static void call_control(mlib_context_l *ctl) {
	mlib_context_t *conf = mlib_context_list_find2(ctl, "sip");
	pj_pool_t *pool = mlib_module_pool(msip_mod());
	mlib_context_val *max, *reg;
	struct sip_ctl *data = pj_pool_zalloc(pool, sizeof(struct sip_ctl));
	max = mlib_context_type_get_value2(conf, "max_call");
	if (max) {
		data->max_call = mlib_context_val_to_int(max);
		mlib_event_handle_t *hand = mlib_event_handle_create(msip_mod(),
				sip_handle_p,
				NULL, data);
		msip_event_register(hand);
	}
	reg = mlib_context_type_get_value2(conf, "auto_retry");
	if (reg) {
		const pj_str_t *rf = mlib_context_val_to_str(reg);
		if (pj_strcmp2(rf, "yes") == 0) {
			mlib_timer_t *entry = mlib_timer_entry_create(msip_mod(), "retry",
					PJ_TRUE, 60 * 1000, timer_active_ua, NULL);
			mlib_util_timer_register(entry);
		}
	}
}
MLIB_LOCAL void _msip_conf(mlib_context_l *ctl) {
	mlib_context_val *val;
	mlib_context_t *conf = mlib_context_list_find2(ctl, "sip");
	if (!conf)
		return;
#if PJSIP_HAS_TLS_TRANSPORT
	val = mlib_context_type_get_value2(conf, "tls");
	if (val) {
		const pj_str_t *st = mlib_context_val_to_str(val);
		if (st && pj_strcmp2(st, "config") == 0) {
			tls_config(ctl);
		}
	}
#endif
	call_control(ctl);
}
