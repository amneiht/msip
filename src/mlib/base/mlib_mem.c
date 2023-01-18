/*
 * mlib_mem.h
 *
 *  Created on: Jan 4, 2023
 *      Author: amneiht
 */

#include <mlib/mem.h>
#include <pj/rbtree.h>
#include "local.h"
static pj_rbtree mem_tree;
#define MLIB_MEM_LOCK

#ifdef MLIB_MEM_LOCK

static pj_lock_t *mem_lock;
static pj_pool_t *pool;
#define _mbegin pj_lock_acquire( mem_lock ); {
#define _mend  } pj_lock_release( mem_lock );

#else

#define _mbegin {
#define _mend  }

#endif

struct core_mem {
	pj_bool_t des;
	int ref;
	mlib_clear_callback clear;
	void *data;
};
static int mem_cmp(const void *k1, const void *k2) {
	long aa = (long) k1;
	long bb = (long) k2;
	return aa - bb;
}

/*
 mlib_mmap_t* mlib_mmap_create(pj_pool_t *pool) {
 pj_rbtree *rb = PJ_POOL_ALLOC_T(pool, pj_rbtree);
 pj_rbtree_init(rb, mem_cmp);
 return (void*) rb;
 }

 // F(p1)=p2
 pj_status_t mlib_mmap_set(pj_pool_t *pool, mlib_mmap_t *map, void *p1, void *p2) {
 pj_rbtree_node *node = pj_rbtree_find((pj_rbtree*) map, p1);
 if (node != NULL)
 return -1;

 node = PJ_POOL_ALLOC_T(pool, pj_rbtree_node);
 node->key = p1;
 node->user_data = p2;
 pj_rbtree_insert((pj_rbtree*) map, node);
 return PJ_TRUE;
 }
 // get p2 from p1
 void* mlib_mmap_get(mlib_mmap_t *map, void *p1) {
 pj_rbtree_node *node = pj_rbtree_find((pj_rbtree*) map, p1);
 if (node == NULL)
 return NULL;
 return node->user_data;
 }

 void* mlib_mmap_unset(mlib_mmap_t *map, void *p1) {
 pj_rbtree_node *node = pj_rbtree_find((pj_rbtree*) map, p1);
 if (node == NULL)
 return NULL;
 void *res = node->user_data;
 pj_rbtree_erase((pj_rbtree*) map, node);
 return res;
 }

 // memory nmap */
MLIB_LOCAL void _mlib_mem_init() {
	pj_rbtree_init(&mem_tree, mem_cmp);
#ifdef MLIB_MEM_LOCK
	pool = mlib_pool_create(NULL, MLIB_POOL_SIZE, MLIB_POOL_SIZE);
	pj_lock_create_recursive_mutex(pool, "mem_lock", &mem_lock);
#endif
}
MLIB_LOCAL void _mlib_mem_close() {
// todo : release all data in tree
#ifdef MLIB_MEM_LOCK
	pj_lock_destroy(mem_lock);
	mlib_pool_release(pool);
#endif

}

static struct core_mem* find_mem(void *data) {
	pj_rbtree_node *rb = pj_rbtree_find(&mem_tree, data);
	if (rb != NULL) {
		return (struct core_mem*) rb->user_data;
	} else
		return NULL;
}
static struct pj_rbtree_node* find_node(void *data) {
	pj_rbtree_node *rb = pj_rbtree_find(&mem_tree, data);
	return rb;
}
pj_status_t mlib_mem_bind(pj_pool_t *pool, void *data,
		mlib_clear_callback clear) {
	if (clear == NULL) {
		return -1;
	}
	struct core_mem *cmd;
	cmd = find_mem(data);
	if (cmd) {
		return PJ_SUCCESS;
	}
	pj_rbtree_node *rb = pj_pool_alloc(pool, sizeof(pj_rbtree_node));
	rb->key = data;
	struct core_mem *cmem = pj_pool_alloc(pool, sizeof(struct core_mem));
	cmem->data = data;
	cmem->clear = clear;
	// khoi tao luong tham chieu ban dau la 1
	cmem->ref = 1;
	cmem->des = PJ_FALSE;
	rb->user_data = cmem;
	pj_rbtree_insert(&mem_tree, rb);
	return PJ_SUCCESS;
}
/**
 * add one to reference counter
 * @param data
 * @return
 */
pj_status_t mlib_mem_add_ref(void *data) {
	struct core_mem *cmd;
	pj_status_t st = -1;
	cmd = find_mem(data);
	_mbegin
		if (cmd) {
			cmd->ref = cmd->ref + 1;
			st = PJ_SUCCESS;
		}

	_mend
	return st;
}

pj_status_t mlib_mem_dec_ref(void *data) {
	struct core_mem *cmd;
	pj_status_t st = -1;
	pj_rbtree_node *rb = find_node(data);
	cmd = (struct core_mem*) rb->user_data;
	_mbegin
		if (cmd) {
			cmd->ref = cmd->ref - 1;
			if (cmd->ref <= 0) {
				pj_rbtree_erase(&mem_tree, rb);
				cmd->clear(data);
			}
			st = PJ_SUCCESS;
		}
// end dec counter
	_mend
	return st;
}
pj_bool_t mlib_mem_is_available(void *data) {
	if (data == NULL)
		return PJ_FALSE;
	struct core_mem *cmd;
	pj_bool_t res = PJ_FALSE;

	cmd = find_mem(data);
	if (cmd) {
		res = !cmd->des;
	}
	return res;
}
int mlib_mem_get_nref(void *data) {
	struct core_mem *cmd;
	cmd = find_mem(data);
	if (cmd)
		return cmd->ref;
	return 0;
}
pj_status_t mlib_mem_is_bind(void *data) {
	struct core_mem *cmd;
	cmd = find_mem(data);
	return cmd != NULL;
}
pj_status_t mlib_mem_mask_destroy(void *data) {
	struct core_mem *cmd;
	pj_status_t st = -1;
	pj_rbtree_node *rb = find_node(data);
	cmd = (struct core_mem*) rb->user_data;
	_mbegin
		if (cmd && !cmd->des) {
			cmd->des = PJ_TRUE;
			if (cmd->ref <= 0) {
				pj_rbtree_erase(&mem_tree, rb);
				cmd->clear(data);
			}
			st = PJ_SUCCESS;
		}

	_mend
	return st;
}

