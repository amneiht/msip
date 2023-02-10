/*
 * mlib_mem.h
 *
 *  Created on: Jan 4, 2023
 *      Author: amneiht
 */

#include <mlib/mem.h>
#include <mlib_local.h>
#include <pj/rbtree.h>

static int mem_cmp(const void *k1, const void *k2) {
	long aa = (long) k1;
	long bb = (long) k2;
	return aa - bb;
}

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
