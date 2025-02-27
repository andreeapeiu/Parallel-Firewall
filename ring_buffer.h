/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_RINGBUFFER_H__
#define __SO_RINGBUFFER_H__

#include <sys/types.h>
#include <pthread.h>
#include "packet.h"  // Include definiția pentru so_packet_t

typedef struct so_ring_buffer_t {
    so_packet_t *data;      // Memoria bufferului circular
    size_t read_pos;        // Poziția de citire
    size_t write_pos;       // Poziția de scriere

    size_t len;             // Dimensiunea curentă a bufferului
    size_t cap;             // Capacitatea maximă a bufferului

    pthread_mutex_t lock;   // Mutex pentru protecția accesului la buffer
    pthread_cond_t not_empty; // Condiție pentru a aștepta când bufferul este gol
    pthread_cond_t not_full;  // Condiție pentru a aștepta când bufferul este plin

    int stop;               // Flag pentru a semnala oprirea
} so_ring_buffer_t;

int     ring_buffer_init(so_ring_buffer_t *rb, size_t cap);
ssize_t ring_buffer_enqueue(so_ring_buffer_t *rb, void *data, size_t size);
ssize_t ring_buffer_dequeue(so_ring_buffer_t *rb, void *data, size_t size);
void    ring_buffer_destroy(so_ring_buffer_t *rb);
void    ring_buffer_stop(so_ring_buffer_t *rb);

#endif /* __SO_RINGBUFFER_H__ */
