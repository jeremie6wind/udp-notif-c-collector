#include <stdint.h>
#include "queue.h"

#ifndef H_LISTENING_WORKER
#define H_LISTENING_WORKER

/**
 * Input given to the listener thread when creating it.
 */
struct listener_thread_input
{
  queue_t *output_queue;      /* The queue used to push the segments outside. */
  uint16_t port;             /* The port to initialize the interface on. */
};

int listener(struct listener_thread_input *in);
void *t_listener(void *in);

#endif