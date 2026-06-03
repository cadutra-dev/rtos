#include "../../include/rtos.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t id;
    uint32_t count;
    uint32_t max_count;
} semaphore_internal_t;

static semaphore_internal_t semaphores[RTOS_MAX_SEMAPHORES];
static uint32_t next_sem_id = 1;

rtos_semaphore_t rtos_semaphore_create(uint32_t initial_count) {
    for (int i = 0; i < RTOS_MAX_SEMAPHORES; i++) {
        if (semaphores[i].id == 0) {
            semaphores[i].id = next_sem_id++;
            semaphores[i].count = initial_count;
            semaphores[i].max_count = initial_count;
            return semaphores[i].id;
        }
    }
    return 0;  /* Sem espaço disponível */
}

int rtos_semaphore_take(rtos_semaphore_t sem, uint32_t timeout_ms) {
    (void)timeout_ms;  /* TODO: implementar timeout */

    for (int i = 0; i < RTOS_MAX_SEMAPHORES; i++) {
        if (semaphores[i].id == sem) {
            if (semaphores[i].count > 0) {
                semaphores[i].count--;
                return RTOS_OK;
            }
            return RTOS_TIMEOUT;  /* Simplificado */
        }
    }
    return RTOS_ERROR;
}

int rtos_semaphore_give(rtos_semaphore_t sem) {
    for (int i = 0; i < RTOS_MAX_SEMAPHORES; i++) {
        if (semaphores[i].id == sem) {
            if (semaphores[i].count < semaphores[i].max_count) {
                semaphores[i].count++;
                return RTOS_OK;
            }
            return RTOS_ERROR;  /* Semáforo cheio */
        }
    }
    return RTOS_ERROR;
}

int rtos_semaphore_delete(rtos_semaphore_t sem) {
    for (int i = 0; i < RTOS_MAX_SEMAPHORES; i++) {
        if (semaphores[i].id == sem) {
            memset(&semaphores[i], 0, sizeof(semaphore_internal_t));
            return RTOS_OK;
        }
    }
    return RTOS_ERROR;
}
