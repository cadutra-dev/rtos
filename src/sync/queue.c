#include "../../include/rtos.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t id;
    void *buffer;
    size_t item_size;
    size_t queue_length;
    size_t head;
    size_t tail;
    size_t count;
} queue_internal_t;

static queue_internal_t queues[RTOS_MAX_QUEUES];
static uint32_t next_queue_id = 1;

rtos_queue_t rtos_queue_create(size_t item_size, size_t queue_length) {
    for (int i = 0; i < RTOS_MAX_QUEUES; i++) {
        if (queues[i].id == 0) {
            queues[i].buffer = rtos_malloc(item_size * queue_length);
            if (queues[i].buffer == NULL) {
                return 0;
            }

            queues[i].id = next_queue_id++;
            queues[i].item_size = item_size;
            queues[i].queue_length = queue_length;
            queues[i].head = 0;
            queues[i].tail = 0;
            queues[i].count = 0;

            return queues[i].id;
        }
    }
    return 0;
}

int rtos_queue_send(rtos_queue_t queue, const void *item, uint32_t timeout_ms) {
    (void)timeout_ms;

    for (int i = 0; i < RTOS_MAX_QUEUES; i++) {
        if (queues[i].id == queue) {
            if (queues[i].count >= queues[i].queue_length) {
                return RTOS_ERROR;  /* Fila cheia */
            }

            /* Copia item para fila */
            void *dest = (void *)((uintptr_t)queues[i].buffer + 
                                  queues[i].tail * queues[i].item_size);
            memcpy(dest, item, queues[i].item_size);

            queues[i].tail = (queues[i].tail + 1) % queues[i].queue_length;
            queues[i].count++;

            return RTOS_OK;
        }
    }
    return RTOS_ERROR;
}

int rtos_queue_receive(rtos_queue_t queue, void *item, uint32_t timeout_ms) {
    (void)timeout_ms;

    for (int i = 0; i < RTOS_MAX_QUEUES; i++) {
        if (queues[i].id == queue) {
            if (queues[i].count == 0) {
                return RTOS_TIMEOUT;  /* Fila vazia */
            }

            /* Copia item da fila */
            void *src = (void *)((uintptr_t)queues[i].buffer + 
                                 queues[i].head * queues[i].item_size);
            memcpy(item, src, queues[i].item_size);

            queues[i].head = (queues[i].head + 1) % queues[i].queue_length;
            queues[i].count--;

            return RTOS_OK;
        }
    }
    return RTOS_ERROR;
}

int rtos_queue_delete(rtos_queue_t queue) {
    for (int i = 0; i < RTOS_MAX_QUEUES; i++) {
        if (queues[i].id == queue) {
            if (queues[i].buffer != NULL) {
                rtos_free(queues[i].buffer);
            }
            memset(&queues[i], 0, sizeof(queue_internal_t));
            return RTOS_OK;
        }
    }
    return RTOS_ERROR;
}
