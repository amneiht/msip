/*
 ============================================================================
 Name        : msip.c
 Author      : amneiht
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include <mlib/mlib.h>
#include <mlib/event.h>
#include <mlib_util/util.h>
#include <msip/msip.h>

static int d = 50;
static void timer_func(mlib_timer_t *entry, void *data) {
	d--;
	if (d > 0)
		mlib_util_event_send(100);
	else
		mlib_util_event_send(MLIB_CLOSE);
}
static void handle_event(void *user_data, int type, mlib_container *event_data) {
	PJ_LOG(4, (MLIB_NAME,"time + event triger"));
}
void test_ua() {
	pj_str_t name = pj_str("500");
	pj_str_t pass = pj_str("500");
	int port = 34001;
	pj_str_t host = pj_str("192.168.100.34");
	pj_str_t transport = pj_str("udp");
	msip_ua *uas = msip_ua_create(&name, &pass, &host, port, &transport);
	msip_ua_register(uas);
}
int main(void) {
	mlib_init();
	msip_init();
	pj_str_t sname = pj_str("fff");
//	mlib_module_t *mod = mlib_module_simple(&sname);
//	// test event
//	mlib_event_handle_t *eh = mlib_event_handle_create(mod, handle_event,
//	NULL, NULL);
//	mlib_util_event_add_handle(eh);
//	// test timer
//	mlib_timer_t *entry = mlib_timer_entry_create(mod, "dlm", PJ_TRUE, 1000,
//			timer_func, NULL);
//	mlib_util_timer_register(entry);

	test_ua();

	mlib_loop();
	mlib_close();

	return 0;
}
