/*
 * console.c
 *
 *  Created on: Feb 1, 2023
 *      Author: amneiht
 */

#include <mlib/mlib.h>
#include <mlib_util/util.h>
#include <fcntl.h>

struct mod_std {
	mlib_timer_t *timer;
};
static void std_timer(mlib_timer_t *entry, void *arg) {
	int c = getc(stdin);
	if (c != EOF) {
		char buffer[1024];
		fgets(buffer + 1, 1024, stdin);
		buffer[0] = c;
		mlib_ui_input(buffer);
		printf("129> ");
	}
}
void console_init_mod() {
	pj_str_t name = pj_str("console");
	mlib_module_t *cmod = mlib_module_simple(&name);
	struct mod_std *mod = PJ_POOL_ALLOC_T(mlib_module_pool(cmod),
			struct mod_std);
	mod->timer = mlib_timer_entry_create(cmod, "std", PJ_TRUE, 200, std_timer,
	NULL);
#if __linux__
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
#endif
	printf("129> ");
	mlib_util_timer_register(mod->timer);
}
