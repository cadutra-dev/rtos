#include "../../include/rtos.h"
#include "../sync/blocking.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint32_t id;
    uint32_t count;
    uint32_t max_count;
    uint32_t waiting_count;     /* Número de tarefas esperando */
} semaphore_internal_t;

static semaphore_internal_t semaphores[RTOS_MAX_SEMAPHORES];
static uint32_t next_sem_id = 1;

rtos_semaphore_t rtos_semaphore_create(uint32_t initial_count) {
    for (int i = 0; i < RTOS_MAX_SEMAPHORES; i++) {
        if (semaphores[i].id == 0) {
            semaphores[i].id = next_sem_id++;
            semaphores[i].count = initial_count;
            semaphores[i].max_count = initial_count;
            semaphores[i].waiting_count = 0;
            return semaphores[i].id;
        }
    }
    return 0;  /* Sem espaço disponível */
}

int rtos_semaphore_take(rtos_semaphore_t sem, uint32_t timeout_ms) {
    for (int i = 0; i < RTOS_MAX_SEMAPHORES; i++) {
        if (semaphores[i].id == sem) {
            /* Se há recurso disponível, toma imediatamente */
            if (semaphores[i].count > 0) {
                semaphores[i].count--;
                return RTOS_OK;
            }
            
            /* Se não há recurso e não quer esperar */
            if (timeout_ms == RTOS_NO_WAIT) {
                return RTOS_TIMEOUT;
            }
            
            /* Bloqueia tarefa */
            rtos_task_handle_t current = rtos_task_get_current();
            blocking_block_task(current, BLOCK_SEMAPHORE, sem, timeout_ms);
            
            semaphores[i].waiting_count++;
            
            /* Cede processador */
            rtos_task_yield();
            
            /* Ao retornar do yield/context-switch, verifica se obteve recurso */
            if (semaphores[i].count > 0) {
                semaphores[i].count--;
                return RTOS_OK;
            }
            
            return RTOS_TIMEOUT;
        }
    }
    return RTOS_ERROR;
}

int rtos_semaphore_give(rtos_semaphore_t sem) {
    for (int i = 0; i < RTOS_MAX_SEMAPHORES; i++) {
        if (semaphores[i].id == sem) {
            if (semaphores[i].count < semaphores[i].max_count) {
                semaphores[i].count++;
                
                /* Se há tarefas esperando, desbloqueia uma */
                if (semaphores[i].waiting_count > 0) {
                    semaphores[i].waiting_count--;
                }
                
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
