/*
 * Copyright: (c) 2006-2016, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        tri_malloc.h
 * Purpose:     Dynamic memory manipulator.
 * History:     07/22/2011, created by jetmotor
 *		Learn from TCPL: A Storage Allocator
 *		Learn from glibc: ptmalloc
 *		12/19/2014, modified by ypzhou, add mem_unit header before every malloc, 
 *		06/03/2015, modified by ypzhou, add memory allocator debug command
 */

#ifndef __TRI_MALLOC_H__
#define __TRI_MALLOC_H__

#include "types.h"
#include "list.h"

#define MEM_UNIT_SIZE	32	/* shall be larger than `sizeof(mem_unit_t)' */
#define MEM_UNIT_BOUND	(MEM_UNIT_SIZE - 1)
#define MEM_UNIT_POWER	5


typedef struct mem_allocator_s {
	uint32_t unit_total;	/* total memory units number */
	uint32_t unit_free;	/* free memory units number */
	uint32_t nr_total;
	uint32_t nr_free;
	list_head_t total;	/* all memory units list, including free and used */
	list_head_t free;	/* all free memory units list */
	void *zone;		/* start of the whole memory in a `MEM_UNIT_SIZE' boundary */
	uint32_t size;
	list_head_t link;	/* mem allocators linkage */

	uint32_t error;
	uint32_t fail;
	const char *name;
} mem_allocator_t;

typedef struct mem_unit_s {
	uint32_t magic;
	uint32_t nr;		/* size of this continuous memory unit */
	list_head_t to_link;	/* all memory units linkage */
	list_head_t fr_link;	/* free memory units linkage */
	uint32_t tag;
} mem_unit_t;


#define tri_malloc_init(allocator, size, zone)	tri_malloc_init2(allocator, size, zone, #allocator + 1)
/* tri_malloc_init2 - Initialize a memory allocator for `tri_malloc' and `tri_free' usage.
 * @allocator: the allocator
 * @size: size of the to-be-managed buffer, which shall exist in HEAP or BSS.
 * @zone: the buffer
 * @name: allocator's name
 */
extern void tri_malloc_init2(mem_allocator_t *allocator, uint32_t size, void *zone, const char *name);
/* tri_malloc - Try to allocate `@size' bytes from `@allocator' and return a pointer to the allocated memory.
 * @allocator: the allocator
 * @size: the requested size
 * @return: NULL if fail
 *          non-NULL if succeed
 */
extern void * tri_malloc(mem_allocator_t *allocator, uint32_t size);
/* tri_free - Free the memory space pointed by `@p', which must be returned by a previous call to `tri_malloc'.
 * @allocator - the allocator
 * @p: the memory space
 */
extern void tri_free(mem_allocator_t *allocator, void *p);


#endif	/* end of __TRI_MALLOC_H__ */
