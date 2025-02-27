// SPDX-License-Identifier: BSD-3-Clause

#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

// Error codes for ring buffer operations
#define RING_BUFFER_SUCCESS 0
#define RING_BUFFER_ERR_NULL_PTR 1
#define RING_BUFFER_ERR_ALLOC 2
#define RING_BUFFER_ERR_MUTEX_INIT 3

#define RING_BUFFER_ERR_FULL -2
#define RING_BUFFER_ERR_STOPPED -3
#define RING_BUFFER_ERR_INVALID -4

// Initialize the ring buffer with the given capacity
int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	if (!ring || cap == 0) {
		fprintf(stderr, "Invalid input: ring buffer pointer or capacity is invalid.\n");
		return RING_BUFFER_ERR_NULL_PTR;
	}

	ring->data = (so_packet_t *)calloc(cap, sizeof(so_packet_t));
	if (!ring->data) {
		fprintf(stderr, "Memory allocation failed during ring buffer initialization.\n");
		return RING_BUFFER_ERR_ALLOC;
	}

	ring->cap = cap;
	ring->len = 0;
	ring->read_pos = 0;
	ring->write_pos = 0;
	ring->stop = 0;

	if (pthread_mutex_init(&ring->lock, NULL) != 0) {
		fprintf(stderr, "Failed to initialize ring buffer mutex.\n");
		free(ring->data);
		return RING_BUFFER_ERR_MUTEX_INIT;
	}

	if (pthread_cond_init(&ring->not_empty, NULL) != 0) {
		fprintf(stderr, "Failed to initialize 'not_empty' condition variable.\n");
		free(ring->data);
		pthread_mutex_destroy(&ring->lock);
		return RING_BUFFER_ERR_MUTEX_INIT;
	}

	if (pthread_cond_init(&ring->not_full, NULL) != 0) {
		fprintf(stderr, "Failed to initialize 'not_full' condition variable.\n");
		free(ring->data);
		pthread_cond_destroy(&ring->not_empty);
		pthread_mutex_destroy(&ring->lock);
		return RING_BUFFER_ERR_MUTEX_INIT;
	}

	return RING_BUFFER_SUCCESS;
}

// Add an element to the ring buffer
ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	if (!ring || !data || size != sizeof(so_packet_t)) {
		fprintf(stderr, "Invalid input: enqueue parameters are incorrect.\n");
		return -1;
	}

	pthread_mutex_lock(&ring->lock);

	// Wait until space becomes available in the buffer
	while (ring->len == ring->cap) {
		if (ring->stop) { // Stop signal detected
			pthread_mutex_unlock(&ring->lock);
			fprintf(stderr, "Enqueue operation aborted: ring buffer is stopped.\n");
			return -1;
		}
		pthread_cond_wait(&ring->not_full, &ring->lock);
	}

	// Insert the data into the buffer
	ring->data[ring->write_pos] = *(so_packet_t *)data; // Direct assignment
	ring->write_pos = (ring->write_pos + 1) % ring->cap; // Advance write position
	ring->len++;

	// Notify threads waiting for data availability
	pthread_cond_signal(&ring->not_empty);

	pthread_mutex_unlock(&ring->lock);

	return (ssize_t)size; // Return the size of the enqueued element
}

// Remove an element from the ring buffer
ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	if (!ring || !data || size != sizeof(so_packet_t)) {
		fprintf(stderr, "Invalid input: dequeue parameters are incorrect.\n");
		return -1;
	}

	pthread_mutex_lock(&ring->lock);

	// Wait until there is data available in the buffer
	while (ring->len == 0) {
		if (ring->stop) { // Stop signal detected
			pthread_mutex_unlock(&ring->lock);
			fprintf(stderr, "Dequeue operation aborted: ring buffer is stopped.\n");
			return -1;
		}
		pthread_cond_wait(&ring->not_empty, &ring->lock);
	}

	// Retrieve the data from the buffer
	memcpy(data, &ring->data[ring->read_pos], size);
	ring->read_pos = (ring->read_pos + 1) % ring->cap; // Advance read position
	ring->len--; // Decrease the buffer size

	// Notify threads waiting for space availability
	pthread_cond_signal(&ring->not_full);

	pthread_mutex_unlock(&ring->lock);

	return (ssize_t)size; // Return the size of the dequeued element
}

// Release resources allocated for the ring buffer
void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	if (!ring) {
		fprintf(stderr, "Attempted to destroy a NULL ring buffer.\n");
		return;
	}

	if (ring->data) {
		free(ring->data);
		ring->data = NULL;
	}

	if (pthread_mutex_destroy(&ring->lock) != 0)
		fprintf(stderr, "Failed to destroy ring buffer mutex.\n");

	if (pthread_cond_destroy(&ring->not_empty) != 0)
		fprintf(stderr, "Failed to destroy 'not_empty' condition variable.\n");

	if (pthread_cond_destroy(&ring->not_full) != 0)
		fprintf(stderr, "Failed to destroy 'not_full' condition variable.\n");
}

// Stop the ring buffer and notify waiting threads
void ring_buffer_stop(so_ring_buffer_t *ring)
{
	if (!ring) {
		fprintf(stderr, "Attempted to stop a NULL ring buffer.\n");
		return;
	}

	pthread_mutex_lock(&ring->lock);

	ring->stop = 1; // Set the stop flag

	// Wake up all threads waiting on condition variables
	pthread_cond_broadcast(&ring->not_empty);
	pthread_cond_broadcast(&ring->not_full);

	pthread_mutex_unlock(&ring->lock);
}
