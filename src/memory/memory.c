#include "../../include/rtos.h"
#include <stdlib.h>
#include <string.h>

#define HEAP_SIZE (16 * 1024)  /* 16 KB de heap */

static uint8_t heap[HEAP_SIZE];
static size_t heap_used = 0;

void memory_init(void) {
    heap_used = 0;
    memset(heap, 0, HEAP_SIZE);
}

void *rtos_malloc(size_t size) {
    if (size == 0 || heap_used + size > HEAP_SIZE) {
        return NULL;
    }

    void *ptr = (void *)&heap[heap_used];
    heap_used += size;

    return ptr;
}

void rtos_free(void *ptr) {
    /* Alocador simples não libera memória */
    (void)ptr;
    /* Em uma implementação real, seria necessário um alocador mais sofisticado */
}
