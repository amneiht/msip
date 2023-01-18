/*
 * mbase.h
 *
 *  Created on: Jan 3, 2023
 *      Author: amneiht
 */

#ifndef MLIB_BASE_H_
#define MLIB_BASE_H_

#include <pj/types.h>
#include <mlib/config.h>
typedef void (*mlib_clear_callback)(void *user_data);
#define MLIB_LOCAL __attribute__ ((visibility ("hidden")))

#define MLIB_NAME (strrchr(__FILE__, MLIB_PATH_SP) ? strrchr(__FILE__,  MLIB_PATH_SP) + 1 : __FILE__)  // file name
#define MLIB_LINE   __LINE__  // line in name
#define MLIB_FUNC	 __func__  // funtion name_

#ifdef _WIN32
#define MLIB_PATH_SP '\\'
#else
#define MLIB_PATH_SP '/'
#endif

#define mlib_rand() pj_rand()

#define mlib_alloca(type) (type *) alloca( sizeof(type))
#define mlib_size_algin(size) ( size  + size % 1024 )

#define MLIB_POOL_SIZE 8192

/* struct declear */

typedef struct {
	int leng;
	void *data[32];
} mlib_container;

/*  endof struct declear */

pj_status_t mlib_init();
void mlib_close();

/* pool funtion */
pj_pool_t* mlib_pool_create(const char *name, int pool_size, int pool_inc);
void mlib_pool_release(pj_pool_t *pool);
pj_pool_factory* mlib_pool_factory();

#endif /* MLIB_BASE_H_ */
