// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

// Thread function for processing packets
void *consumer_thread(void *arg)
{
	so_consumer_ctx_t *ctx = (so_consumer_ctx_t *)arg;
	char buffer[PKT_SZ];
	ssize_t sz;

	for (;;) {
		// Dequeue a packet
		sz = ring_buffer_dequeue(ctx->ring_buffer, buffer, PKT_SZ);

		if (sz <= 0) {
			if (ctx->ring_buffer->stop) {
				fprintf(stderr, "Stopping consumer thread.\n");
				break;
			}
			continue; // Retry if no packet is available
		}

		struct so_packet_t *pkt = (struct so_packet_t *)buffer;
		unsigned long timestamp = pkt->hdr.timestamp;

		// Process the packet
		int action = process_packet(pkt);
		unsigned long hash = packet_hash(pkt);

		// Prepare output
		char out_buf[PKT_SZ];
		int len = snprintf(out_buf, sizeof(out_buf), "%s %016lx %lu\n", RES_TO_STR(action), hash, timestamp);

		if (len <= 0) {
			fprintf(stderr, "Error formatting output.\n");
			continue; // Skip if snprintf fails
		}

		// Write to file
		pthread_mutex_lock(ctx->file_lock);
		ssize_t written = write(ctx->out_fd, out_buf, len);

		pthread_mutex_unlock(ctx->file_lock);

		if (written < 0)
			perror("Write error");
		else if (written != len)
			fprintf(stderr, "Partial write detected.\n");
	}

	fprintf(stderr, "Consumer thread exiting.\n");
	pthread_exit((void *)EXIT_SUCCESS);
}

// Allocate and initialize a consumer context
static so_consumer_ctx_t *initialize_context(struct so_ring_buffer_t *rb, const char *out_filename,
											 int out_fd, pthread_mutex_t *file_lock)
{
	so_consumer_ctx_t *ctx = (so_consumer_ctx_t *)malloc(sizeof(so_consumer_ctx_t));

	if (!ctx) {
		perror("Allocation error");
		return NULL;
	}

	ctx->ring_buffer = rb;
	ctx->out_filename = out_filename;
	ctx->out_fd = out_fd;
	ctx->file_lock = file_lock;

	return ctx;
}

// Cleanup threads and resources
static void cleanup_threads(pthread_t *tids, int threads_created, int out_fd)
{
	for (int i = 0; i < threads_created; i++) {
		pthread_cancel(tids[i]);
		pthread_join(tids[i], NULL);
	}
	if (out_fd >= 0)
		close(out_fd);
}

// Create consumer threads
int create_consumers(pthread_t *tids, int num_consumers, struct so_ring_buffer_t *rb, const char *out_filename)
{
	static pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

	int out_fd = open(out_filename, O_RDWR | O_CREAT | O_APPEND, 0666);

	if (out_fd < 0) {
		perror("File open error");
		return -1;
	}

	int threads_created = 0;

	for (int i = 0; i < num_consumers; i++) {
		so_consumer_ctx_t *ctx = initialize_context(rb, out_filename, out_fd, &file_lock);

		if (!ctx) {
			cleanup_threads(tids, threads_created, out_fd);
			return -1;
		}

		if (pthread_create(&tids[i], NULL, consumer_thread, ctx) != 0) {
			perror("Thread creation error");
			free(ctx); // Free memory for failed context
			cleanup_threads(tids, threads_created, out_fd);
			return -1;
		}

		threads_created++;
	}

	return threads_created;
}

// Join consumer threads and clean up
void join_consumers(pthread_t *tids, int num_consumers, so_consumer_ctx_t *ctx)
{
	for (int i = 0; i < num_consumers; i++)
		pthread_join(tids[i], NULL);
	close(ctx[0].out_fd);
	free(ctx);
}
