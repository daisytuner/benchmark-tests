#include "queue.h"

#include <stdlib.h>
#include <math.h>
#include <stdatomic.h>
#include <omp.h>

//! From header
void queue_init(queue_t* q) {
    q->data = (vec3d_t*) malloc(8 * sizeof(vec3d_t));
    q->data_size = 8;
    q->start = 0;
    q->end = 0;
}

//! From header
void queue_free(queue_t* q) {
    free(q->data);
}

//! From header
size_t queue_get_size(queue_t* q) {
    return (q->end - q->start) % q->data_size;
}

//! From header
void queue_push(queue_t* q, vec3d_t elem) {
    if(queue_get_size(q) == q->data_size - 1) {
        vec3d_t* data_new = (vec3d_t*) malloc(2 * q->data_size * sizeof(vec3d_t));
        size_t j = 0;
        for(size_t i = q->start; i != q->end; i = (i + 1) % q->data_size, ++j) {
            data_new[j] = q->data[i];
        }
        q->data_size *= 2;
        q->start = 0;
        q->end = j;
        free(q->data);
        q->data = data_new;
    }
    q->data[q->end] = elem;
    q->end = (q->end + 1) % q->data_size;
}

//! From header
vec3d_t queue_pop(queue_t* q) {
    vec3d_t elem = { .x = -1, .y = -1, .z = -1 };
    if(q->start != q->end) {
        elem = q->data[q->start];
        q->start = (q->start + 1) % q->data_size;
    }
    return elem;
}

//! From header
void pqueue_init(pqueue_t* q) {
    q->data = (_Atomic(vec3d_t)*) malloc(8 * sizeof(_Atomic(vec3d_t)));
    q->data_size = 8;
    q->start = 0;
    q->end = 0;
    omp_init_lock(&q->extendLock);
    omp_init_lock(&q->writeLock);
    omp_init_lock(&q->readLock);
}

//! From header
void pqueue_free(pqueue_t* q) {
    omp_destroy_lock(&q->extendLock);
    omp_destroy_lock(&q->writeLock);
    omp_destroy_lock(&q->readLock);
    free(q->data);
}

//! From header
size_t pqueue_get_size(pqueue_t* q) {
    return (q->end - q->start) % q->data_size;
}

//! From header
void pqueue_push(pqueue_t* q, vec3d_t elem) {
    omp_set_lock(&q->extendLock);
    size_t tmp_data_size = q->data_size - 1;
    size_t tmp_queue_size = pqueue_get_size(q);
    if(tmp_queue_size == tmp_data_size) {
        _Atomic(vec3d_t)* data_new = (_Atomic(vec3d_t)*) malloc(2 * q->data_size * sizeof(_Atomic(vec3d_t)));
        size_t j = 0;
        for(size_t i = q->start; i != q->end; i = (i + 1) % q->data_size, ++j) {
            data_new[j] = q->data[i];
        }
        q->data_size *= 2;
        q->start = 0;
        q->end = j;
        free(q->data);
        q->data = data_new;
    }
    omp_unset_lock(&q->extendLock);
    omp_set_lock(&q->writeLock);
    q->data[q->end] = elem;
    q->end = (q->end + 1) % q->data_size;
    omp_unset_lock(&q->writeLock);
}

//! From header
vec3d_t pqueue_pop(pqueue_t* q) {
    vec3d_t elem = { .x = -1, .y = -1, .z = -1 };
    omp_set_lock(&q->readLock);
    if(q->start != q->end) {
        elem = q->data[q->start];
        q->start = (q->start + 1) % q->data_size;
    }
    omp_unset_lock(&q->readLock);
    return elem;
}
