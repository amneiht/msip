/*
 * msip_ua.c
 *
 *  Created on: Jan 28, 2023
 *      Author: amneiht
 */

#include <pjsip.h>
#include <pjsip_ua.h>

#include "msip_local.h"
#include <mlib/mem.h>

typedef struct ua_ctl {
	pj_list ua_list;
	pj_lock_t *ua_lock;
} ua_ctl;
static struct ua_ctl *ua_control;

static void ua_clear(void *arg) {
	ua_ctl *uas = arg;
	pj_lock_acquire(uas->ua_lock);
	msip_ua *ua = (msip_ua*) uas->ua_list.next;
	msip_ua *last = (msip_ua*) uas->ua_list.prev;
	msip_ua *p;
	while (ua != last) {
		p = (msip_ua*) ua->next;
		msip_ua_destroy(ua);
		ua = p;
	}
	pj_lock_release(uas->ua_lock);
	pj_lock_destroy(uas->ua_lock);
}
MLIB_LOCAL void _init_sip_ua() {
	if (_msip_obj == NULL) {
		PJ_LOG(1, (MLIB_NAME,"msip is not init"));
		return;
	}
	mlib_module_t *mod = _msip_obj->mod;
	pj_pool_t *pool = mlib_module_pool(mod);
	ua_control = mlib_modctl_alloc(_msip_obj->mod, sizeof(ua_ctl), ua_clear);
	pj_list_init(&ua_control->ua_list);
	pj_lock_create_recursive_mutex(pool, "sipua", &ua_control->ua_lock);
}

const pj_str_t* msip_ua_get_transport(msip_ua *ua) {
	return &ua->transport;
}
//create ua
static void sip_ua_clear(void *arg) {
	msip_ua *uas = arg;
	if (uas->login.regc != NULL) {
		pjsip_regc_destroy(uas->login.regc);
	}
	mlib_pool_release(uas->pool);
	mlib_mem_dec_ref(ua_control);
}
msip_ua* msip_ua_create(const pj_str_t *user, const pj_str_t *pass,
		const pj_str_t *host, const int port, pj_str_t *transport) {
	pj_pool_t *pool = mlib_pool_create(NULL, 2048, 2048);
	msip_ua *uas = (msip_ua*) pj_pool_zalloc(pool, sizeof(msip_ua));
	uas->pool = pool;
// create cred
	pj_strdup_with_null(pool, &uas->login.cred.realm, host);
	pj_strdup2_with_null(pool, &uas->login.cred.scheme, "digest");
	pj_strdup_with_null(pool, &uas->login.cred.username, user);
	pj_strdup_with_null(pool, &uas->login.cred.data, pass);
	uas->login.cred.data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
// contact buffer
	uas->contact.uid.ptr = pj_pool_alloc(pool, PJ_GUID_STRING_LENGTH + 1);
	uas->contact.uid.ptr[PJ_GUID_STRING_LENGTH] = 0;
	pj_generate_unique_string(&uas->contact.uid);
	uas->contact.last_contact.ptr = pj_pool_alloc(pool, 500);
// set param for login
	uas->login.state = msip_state_null;
	uas->login.login_time = 60;
	uas->port = port;
// for invite session

// transport
	if (transport != NULL)
		pj_strdup(pool, &uas->transport, transport);
	else
		pj_strdup2(pool, &uas->transport, "tcp");

	mlib_mem_bind(pool, uas, sip_ua_clear);
	pj_list_init(&uas->call);
	mlib_mem_add_ref(ua_control);
	pj_list_insert_before(&ua_control->ua_list, uas);
	return uas;
}
pj_bool_t msip_ua_destroy(msip_ua *ua) {
	mlib_mem_mask_destroy(ua);
	return PJ_TRUE;
}

//find user_agent
static pj_bool_t find_by_uri(void *value, const pj_list_type *node) {
	const msip_ua *uas = node;
	pjsip_sip_uri *suri = value;
	return pj_strcmp(&suri->user, &uas->login.cred.username)
			|| pj_strcmp(&suri->host, &uas->login.cred.realm);
}
msip_ua* msip_ua_find_by_uri(pjsip_uri *uri) {
	if (PJSIP_URI_SCHEME_IS_SIP(uri) || PJSIP_URI_SCHEME_IS_SIPS(uri)) {
		pjsip_sip_uri *suri = (pjsip_sip_uri*) pjsip_uri_get_uri(uri);
		return (msip_ua*) pj_list_search(&ua_control->ua_list, suri,
				find_by_uri);
	} else {
		return NULL;
	}
}
msip_ua* msip_ua_find_by_msg(pjsip_msg *msg) {
	pjsip_to_hdr *hdr = PJSIP_MSG_TO_HDR(msg);
	if (hdr == NULL)
		return NULL;
	pjsip_uri *uri = hdr->uri;
	return msip_ua_find_by_uri(uri);
}
static pj_bool_t find_by_name(void *value, const pj_list_type *node) {
	const msip_ua *uas = node;
	pj_str_t *str = value;
	return pj_strcmp(str, &uas->login.cred.username);
}
msip_ua* msip_ua_find_by_name(pj_str_t *name) {
	return (msip_ua*) pj_list_search(&ua_control->ua_list, name, find_by_name);
}

// uri help
pj_bool_t msip_ua_print_uri(msip_ua *ua, pj_str_t *uri) {

	if (ua->pool == 0)
		uri->slen = sprintf(uri->ptr, "<sip:%.*s@%.*s>",
				(int) ua->login.cred.username.slen, ua->login.cred.username.ptr,
				(int) ua->login.cred.realm.slen, ua->login.cred.realm.ptr);
	else
		uri->slen = sprintf(uri->ptr, "<sip:%.*s@%.*s:%d>",
				(int) ua->login.cred.username.slen, ua->login.cred.username.ptr,
				(int) ua->login.cred.realm.slen, ua->login.cred.realm.ptr,
				ua->port);

	return PJ_TRUE;
}
static pj_bool_t checkuri(const pj_str_t *name) {
	int str = name->slen;
	for (int i = 0; i < str; i++) {
		if (name->ptr[i] == '@')
			return PJ_TRUE;
	}
	return PJ_FALSE;
}
pj_bool_t msip_ua_print_call_uri(msip_ua *ua, const pj_str_t *callee,
		pj_str_t *out) {
	out->slen = 0;
	if (checkuri(callee)) {
		out->slen = sprintf(out->ptr, "<sip:%.*s>", (int) callee->slen,
				callee->ptr);
	} else {
		if (ua->port == 0)
			out->slen = sprintf(out->ptr, "<sip:%.*s@%.*s>", (int) callee->slen,
					callee->ptr, (int) ua->login.cred.realm.slen,
					ua->login.cred.realm.ptr);
		else
			out->slen = sprintf(out->ptr, "<sip:%.*s@%.*s:%d>",
					(int) callee->slen, callee->ptr,
					(int) ua->login.cred.realm.slen, ua->login.cred.realm.ptr,
					ua->port);
	}

	return PJ_TRUE;
}
pj_bool_t msip_ua_print_contact(msip_ua *ua, pj_str_t *conf) {
	pj_sockaddr addr;
	pj_gethostip(pj_AF_INET(), &addr);
	char *hostip = pj_inet_ntoa(addr.ipv4.sin_addr);
	const pj_str_t *tran = &ua->transport;
#if 0
	int port = msip_ua_transport_port(ua);
	conf->slen = sprintf(conf->ptr,
			"<sip:%.*s@%s:%d;transport=%.*s>;+sip.instance=\"<urn:uuid:%s>\"",
			(int) ua->login.cred.username.slen, ua->login.cred.username.ptr,
			hostip, port, (int) tran->slen, tran->ptr, ua->contact.uid.ptr);
#else
	conf->slen = sprintf(conf->ptr,
			"<sip:%.*s@%s;transport=%.*s>;+sip.instance=\"<urn:uuid:%s>\"",
			(int) ua->login.cred.username.slen, ua->login.cred.username.ptr,
			hostip, (int) tran->slen, tran->ptr, ua->contact.uid.ptr);
#endif
	return PJ_TRUE;
}

// print availe ua
void msip_ua_print() {
	pj_list *ua_list = &ua_control->ua_list;
	msip_ua *p = ua_list->next;
	msip_ua *last = ua_list->prev;
	pj_str_t uri;
	uri.ptr = alloca(300);
	while (p != last) {
		msip_ua_print_uri(p, &uri);
		PJ_LOG(1, (MLIB_NAME,"ua :\"%s\"",uri.ptr));
		p = (msip_ua*) p->next;
	}
}
MLIB_LOCAL int msip_ua_transport_port(msip_ua *ua) {
	struct msip_system *sip = _msip_obj;
	int port = 0;
	if (pj_strcmp2(&ua->transport, "tcp") == 0 && sip->transport.tcp != 0) {
		port = sip->transport.tcp->addr_name.port;
	} else if (pj_strcmp2(&ua->transport, "udp") == 0
			&& sip->transport.udp != 0) {
		port = sip->transport.udp->local_name.port;
	}
	return port;
}

//void msip_ua_print2(acore_ui_output_p out, void *ui_data) {
//	pj_list *ua_list = ua->ua_list;
//	msip_ua *p = ua_list->next;
//	msip_ua *last = ua_list->prev;
//	char buff2[300];
//	pj_str_t pua;
//	pua.ptr = buff2;
//	pj_str_t uri;
//	uri.ptr = alloca(300);
//	while (p != last) {
//		msip_ua_print_uri(p, &uri);
//		pua.slen = sprintf(buff2, "ua :\"%s\"", uri.ptr);
//		out(ui_data, &pua);
//		p = p->next;
//	}
//}

