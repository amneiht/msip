/*
 * test_call.c
 *
 *  Created on: Feb 1, 2023
 *      Author: amneiht
 */

#include <msip/inv.h>

static pj_bool_t test_match_media(const pj_str_t *media);
static pj_bool_t test_match_sdp(const pjmedia_sdp_session *sdp);
static pj_bool_t test_media_update(msip_call *call, void *mod_data);
static pj_bool_t test_state_changed(msip_call *call, enum pjsip_inv_state state,
		void *mod_data);
static pjmedia_sdp_session* test_accept_sdp(msip_call *call,
		pjmedia_sdp_session *remote_sdp, void *user_data, pj_str_t *res);
static pjmedia_sdp_session* test_inv_sdp(msip_call *call, void *user_data,
		pj_str_t *res);
static void test_call_iscomming(msip_call *call, void *user_data) {
	msip_call_accept(call);
}

static pj_bool_t test_match_media(const pj_str_t *media) {
	return PJ_TRUE;
}
static pj_bool_t test_match_sdp(const pjmedia_sdp_session *sdp) {
	return PJ_TRUE;
}
static pj_bool_t test_media_update(msip_call *call, void *mod_data) {

	return PJ_TRUE;
}
static pj_bool_t test_state_changed(msip_call *call, enum pjsip_inv_state state,
		void *mod_data) {
	msip_ua *uas = msip_call_ua(call);
	char buff[200];
	pj_str_t d = { buff, 200 };
	msip_ua_print_uri(uas, &d);
	PJ_LOG(2, ("test" , "Ua:%s state %s",d.ptr,pjsip_inv_state_name(state)));
	return PJ_TRUE;
}
static pjmedia_sdp_session* sample_sdp(msip_call *call) {
	pjmedia_sdp_session *sdp;
	pjmedia_sdp_media *m;
	pjmedia_sdp_attr *attr;
	pj_time_val tv;
	pj_pool_t *pool = msip_call_inv(call)->pool;

	sdp = pj_pool_zalloc(pool, sizeof(pjmedia_sdp_session));
	pj_sockaddr addr;
	pj_gethostip(pj_AF_INET(), &addr);
	char *hostip = pj_inet_ntoa(addr.ipv4.sin_addr);

	pj_gettimeofday(&tv);

	sdp->origin.user = *msip_call_get_callee(call);
	sdp->origin.version = sdp->origin.id = tv.sec + 2208988800UL;
	pj_strdup2(pool, &sdp->origin.net_type, "IN");
	pj_strdup2(pool, &sdp->origin.addr_type, "IP4");
	sdp->origin.addr = pj_str(hostip);
	pj_strdup2(pool, &sdp->name, "pika");

	sdp->conn = pj_pool_zalloc(pool, sizeof(pjmedia_sdp_conn));
	pj_strdup2(pool, &sdp->conn->net_type, "IN");
	pj_strdup2(pool, &sdp->conn->addr_type, "IP4");
	sdp->conn->addr = pj_str(hostip);

	/* sDP time and attributes. */
	sdp->time.start = sdp->time.stop = 0;
	sdp->attr_count = 0;

	/* standard media info: */
	sdp->media_count = 1;
	m = pj_pool_zalloc(pool, sizeof(pjmedia_sdp_media));
	sdp->media[0] = m;

	pj_strdup2(pool, &m->desc.media, "message");
	m->desc.port = 444;
	m->desc.port_count = 1;
	pj_strdup2(pool, &m->desc.transport, "TCP/TLS/MSRP");
	m->attr_count = 0;
	attr = pj_pool_zalloc(pool, sizeof(pjmedia_sdp_attr));

	pj_strdup2(pool, &attr->name, "accept-types");
	pj_strdup2(pool, &attr->value, "message/cpim");
	m->attr[m->attr_count++] = attr;

	/* add fmt to pass sdp validate*/
	m->desc.fmt[m->desc.fmt_count++] = pj_str("*");

	attr = pj_pool_zalloc(pool, sizeof(pjmedia_sdp_attr));
	//	attr->name = pj_str("rtpmap");
	//	attr->value = pj_str("* message");
	pj_strdup2(pool, &attr->name, "rtpmap");
	pj_strdup2(pool, &attr->value, "* message");
	m->attr[m->attr_count++] = attr;

	return sdp;
}
static pjmedia_sdp_session* test_accept_sdp(msip_call *call,
		pjmedia_sdp_session *remote_sdp, void *user_data, pj_str_t *res) {
	return sample_sdp(call);
}
static pjmedia_sdp_session* test_inv_sdp(msip_call *call, void *user_data,
		pj_str_t *res) {
	return sample_sdp(call);
}

static void ui_test(void *ui_data, const pj_str_t *input) {
	pj_str_t name = pj_str("5001");
	pj_str_t name2 = pj_str("5002");
	msip_ua *sua = msip_ua_find_by_name(&name);
	msip_ua_call(sua, &name2, "pika", NULL);
}
static void call_close(void *ui_data, const pj_str_t *input) {
	pj_str_t name = pj_str("5001");
	msip_ua *sua = msip_ua_find_by_name(&name);
	pj_list *lcall = msip_ua_list_call(sua);
	msip_call_close((msip_call*) lcall->next);
}
static mlib_command mctl[] = { { "test", "call", ui_test, NULL }, { "test",
		"close", call_close, NULL } };

void test_mod_init() {
	struct msip_media_obj obs;
	pj_str_t name = pj_str("test_call");
	mlib_module_t *mod = mlib_module_simple(&name);
	obs.module_name = name;
	obs.inv_sdp = test_inv_sdp;
	obs.accept_sdp = test_accept_sdp;
	obs.match_media = test_match_media;
	obs.match_sdp = test_match_sdp;

	obs.call_iscoming = test_call_iscomming;
	obs.on_media_update = test_media_update;
	obs.on_state_changed = test_state_changed;
	obs.on_media_update = test_media_update;

	obs.pri = 2;

	msip_media_register(mod, &obs, NULL);

	mlib_command_register(mod, mctl, 2);
}
