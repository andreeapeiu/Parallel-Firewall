// SPDX-License-Identifier: BSD-3-Clause

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__

#include <pthread.h>
#include "ring_buffer.h"
#include "packet.h"

typedef struct so_consumer_ctx_t {
    struct so_ring_buffer_t *ring_buffer;  /**< Pointer to the shared ring buffer. */
    const char *out_filename;             /**< Name of the output file. */
    int out_fd;                           /**< Shared file descriptor for the output file. */
    pthread_mutex_t *file_lock;           /**< Lock for synchronizing file writes. */
    pthread_mutex_t *timestamp_lock;      /**< Lock for synchronizing timestamp processing. */
    unsigned long last_processed_timestamp; /**< Timestamp of the last processed packet. */
} so_consumer_ctx_t;

int create_consumers(pthread_t *tids,
                     int num_consumers,
                     struct so_ring_buffer_t *rb,
                     const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
