#include "../../include/rtos.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t id;
    int locked;  /* 0 = unlocked, >0 = locked */
    rtos_task_handle_t owner;
} mutex_internal_t;

static mutex_internal_t mutexes[RTOS_MAX_MUTEXES];
static uint32_t next_mutex_id = 1;

rtos_mutex_t rtos_mutex_create(void) {
    for (int i = 0; i < RTOS_MAX_MUTEXES; i++) {
        if (mutexes[i].id == 0) {
            mutexes[i].id = next_mutex_id++;
            mutexes[i].locked = 0;
            mutexes[i].owner = 0;
            return mutexes[i].id;
        }
    }
    return 0;
}

int rtos_mutex_lock(rtos_mutex_t mtx, uint32_t timeout_ms) {
    (void)timeout_ms;

    for (int i = 0; i < RTOS_MAX_MUTEXES; i++) {
        if (mutexes[i].id == mtx) {
            if (mutexes[i].locked == 0) {
                mutexes[i].locked = 1;
                mutexes[i].owner = rtos_task_get_current();
                return RTOS_OK;
            }
            return RTOS_TIMEOUT;  /* Simplificado */
        }
    }
    return RTOS_ERROR;
}

int rtos_mutex_unlock(rtos_mutex_t mtx) {
    for (int i = 0; i < RTOS_MAX_MUTEXES; i++) {
        if (mutexes[i].id == mtx) {
            if (mutexes[i].locked && mutexes[i].owner == rtos_task_get_current()) {
                mutexes[i].locked = 0;
                mutexes[i].owner = 0;
                return RTOS_OK;
            }
            return RTOS_ERROR;  /* Não é o proprietário */
        }
    }
    return RTOS_ERROR;
}

int rtos_mutex_delete(rtos_mutex_t mtx) {
    for (int i = 0; i < RTOS_MAX_MUTEXES; i++) {
        if (mutexes[i].id == mtx) {
            memset(&mutexes[i], 0, sizeof(mutex_internal_t));
            return RTOS_OK;
        }
    }
    return RTOS_ERROR;
}
