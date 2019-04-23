/*
 * Copyright (C) 2007 Intel Corporation
 * Copyright (C) 2019 Ralf Zerres <ralf.zerres@networkx.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef OBJECT_HEAP_H
#define OBJECT_HEAP_H

/* do we need to inline the mutexes?
 * if so, we have to create v4l2_mutex.h ... */
#include <pthread.h>

/*
 * Values
 */

#define OBJECT_HEAP_OFFSET_MASK					0x7F000000
#define OBJECT_HEAP_ID_MASK					0x00FFFFFF

#define OBJECT_HEAP_LAST					-1
#define OBJECT_HEAP_ALLOCATED					-2

/*
 * Structures
 */

struct object_base {
	int id;
	int next_free;
};

struct object_heap {
	pthread_mutex_t mutex;
	int object_size;
	int id_offset;
	int next_free;
	int heap_size;
	int heap_increment;
	void **bucket;
	int num_buckets;
};

/*
 * Functions
 */

/*
 * Return 0 on success, -1 on error
 */
int object_heap_init(struct object_heap *heap, int object_size, int id_offset);

/*
 * Allocates an object
 * Returns the object ID on success, returns -1 on error
 */
int object_heap_allocate(struct object_heap *heap);

/*
 * Lookup an allocated object by object ID
 * Returns a pointer to the object on success, returns NULL on error
 */
struct object_base *object_heap_lookup(struct object_heap *heap, int id);

/*
 * Iterate over all objects in the heap.
 * Returns a pointer to the first object on the heap, returns NULL if heap is empty.
 */
struct object_base *object_heap_first(struct object_heap *heap, int *iterator);

/*
 * Iterate over all objects in the heap.
 * Returns a pointer to the next object on the heap, returns NULL if heap is empty.
 */
struct object_base *object_heap_next(struct object_heap *heap, int *iterator);

/*
 * Frees an object
 */
void object_heap_free(struct object_heap *heap, struct object_base *object);

/*
 * Destroys a heap, the heap must be empty.
 */
void object_heap_destroy(struct object_heap *heap);

#endif
