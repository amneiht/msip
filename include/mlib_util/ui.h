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

typedef struct mlib_ui_data {
	char **argv;
	int argc;
} mlib_ui_data;
#endif /* MLIB_UTIL_UI_H_ */
