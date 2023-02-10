/*
 * ui.h
 *
 *  Created on: Jan 28, 2023
 *      Author: amneiht
 */

#ifndef MLIB_UTIL_UI_H_
#define MLIB_UTIL_UI_H_

#include <mlib/module.h>

typedef void (*mlib_ui_output_p)(void *ui_data, const pj_str_t *out);
typedef void (*mlib_ui_handle_p)(void *user_data, pj_str_t *info,
		mlib_ui_output_p out, void *ui_data);

typedef void (*mlib_ui_func)(void *ui_data, const pj_str_t *input);

typedef struct mlib_ui_data {
	char **argv;
	int argc;
} mlib_ui_data;

typedef struct mlib_command_list mlib_command_list;
typedef struct mlib_command {
	const char *mod;
	const char *name;
	mlib_ui_func func;
	void *data;
} mlib_command;

void mlib_command_register(mlib_module_t *mod, const mlib_command *data,
		int leng);
void mlib_command_unregister(const mlib_command *data, int leng);
void mlib_ui_input(const char *input);
#endif /* MLIB_UTIL_UI_H_ */
