#include <stddef.h>
#include <stdatomic.h>
#include <omp.h>

#ifndef _QUEUE_INCLUDE_
#define _QUEUE_INCLUDE_

typedef struct {
    int x, y, z;
} vec3d_t;

typedef struct {
    vec3d_t* data;
    size_t data_size;
    size_t start;
    size_t end;
} queue_t;

typedef struct {
    _Atomic(vec3d_t)* data;
    atomic_size_t data_size;
    atomic_size_t start;
    atomic_size_t end;
    omp_lock_t extendLock;
    omp_lock_t writeLock;
    omp_lock_t readLock;
} pqueue_t;

//! Initialize a queue
void queue_init(queue_t* q);

//! Release the data of a queue
void queue_free(queue_t* q);

//! Returns the number of elements in the queue
size_t queue_get_size(queue_t* q);

//! Pushes an element into the queue
void queue_push(queue_t* q, vec3d_t elem);

//! Pops an element out of the queue
vec3d_t queue_pop(queue_t* q);

//! Initialize a parallel queue
void pqueue_init(pqueue_t* q);

//! Release the data of a parallel queue
void pqueue_free(pqueue_t* q);

//! Returns the number of elements in the parallel queue
size_t pqueue_get_size(pqueue_t* q);

//! Pushes an element into the parallel queue
void pqueue_push(pqueue_t* q, vec3d_t elem);

//! Pops an element out of the parallel queue
vec3d_t pqueue_pop(pqueue_t* q);

#endif
