#include "../../include/rtos.h"
#include "../sync/blocking.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint32_t id;
    int locked;                 /* 0 = unlocked, >0 = locked */
    rtos_task_handle_t owner;
    uint32_t waiting_count;     /* Número de tarefas esperando */
} mutex_internal_t;

static mutex_internal_t mutexes[RTOS_MAX_MUTEXES];
static uint32_t next_mutex_id = 1;

rtos_mutex_t rtos_mutex_create(void) {
    for (int i = 0; i < RTOS_MAX_MUTEXES; i++) {
        if (mutexes[i].id == 0) {
            mutexes[i].id = next_mutex_id++;
            mutexes[i].locked = 0;
            mutexes[i].owner = 0;
            mutexes[i].waiting_count = 0;
            return mutexes[i].id;
        }
    }
    return 0;
}

int rtos_mutex_lock(rtos_mutex_t mtx, uint32_t timeout_ms) {
    for (int i = 0; i < RTOS_MAX_MUTEXES; i++) {
        if (mutexes[i].id == mtx) {
            rtos_task_handle_t current = rtos_task_get_current();
            
            /* Se não está bloqueado, bloqueia imediatamente */
            if (mutexes[i].locked == 0) {
                mutexes[i].locked = 1;
                mutexes[i].owner = current;
                return RTOS_OK;
            }
            
            /* Se já é o proprietário (reentrant), erro */
            if (mutexes[i].owner == current) {
                printf("[RTOS Mutex] AVISO: Deadlock detectado (reentrant lock)\n");
                return RTOS_ERROR;
            }
            
            /* Se não quer esperar */
            if (timeout_ms == RTOS_NO_WAIT) {
                return RTOS_TIMEOUT;
            }
            
            /* Bloqueia tarefa */
            blocking_block_task(current, BLOCK_MUTEX, mtx, timeout_ms);
            mutexes[i].waiting_count++;
            
            /* Cede processador */
            rtos_task_yield();
            
            /* Ao retornar, verifica se conseguiu o lock */
            if (mutexes[i].locked == 0 || mutexes[i].owner == current) {
                mutexes[i].locked = 1;
                mutexes[i].owner = current;
                return RTOS_OK;
            }
            
            return RTOS_TIMEOUT;
        }
    }
    return RTOS_ERROR;
}

int rtos_mutex_unlock(rtos_mutex_t mtx) {
    for (int i = 0; i < RTOS_MAX_MUTEXES; i++) {
        if (mutexes[i].id == mtx) {
            rtos_task_handle_t current = rtos_task_get_current();
            
            if (mutexes[i].locked && mutexes[i].owner == current) {
                mutexes[i].locked = 0;
                mutexes[i].owner = 0;
                
                /* Se há tarefas esperando, desbloqueia uma */
                if (mutexes[i].waiting_count > 0) {
                    mutexes[i].waiting_count--;
                }
                
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
