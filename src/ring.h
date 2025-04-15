#ifndef RING_H
#define RING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/types.h>

#define LEN_MESSAGE 255

typedef struct {
    u_int8_t data[LEN_MESSAGE];
    u_int16_t hash;
    u_int8_t size;
    u_int8_t type;
} message_t;

typedef struct ring_node {
    int32_t shmid_curr;
    int32_t shmid_next;
    int32_t shmid_prev;
    message_t message[LEN_MESSAGE];
    bool flag_is_busy;
} node_t;

typedef struct ring_t {
    int32_t shmid;
    size_t consumed;
    size_t produced;
    size_t free_slots;
    int32_t shmid_begin;
    int32_t shmid_tail;
} ring_t;

ring_t *init_ring();

void clear_buff(ring_t *);

message_t *pop_message(ring_t *);

node_t *create_node();

void allocate_node(ring_t **begin);

void push_message(ring_t*ring, message_t *message);

#endif //RING_H