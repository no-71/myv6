#ifndef CIRCULAR_QUEUE_H_
#define CIRCULAR_QUEUE_H_

#include "config/basic_types.h"

struct circular_queue {
    size_t begin;
    size_t end;
    size_t size;
    unsigned char data[];
};

static inline int ccqueue_empty(struct circular_queue *q)
{
    return q->begin == q->end;
}

static inline int ccqueue_full(struct circular_queue *q)
{
    return q->end - q->begin == q->size;
}

static inline char ccqueue_get(struct circular_queue *q, size_t i)
{
    return q->data[i % q->size];
}

static inline void ccqueue_write(struct circular_queue *q, size_t i, char val)
{
    q->data[i % q->size] = val;
}

#endif
