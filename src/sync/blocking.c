#include "../include/sync_blocking.h"
#include "../../include/rtos.h"
#include "../scheduler/task.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================
   Variáveis Globais
   ============================================ */

/* Fila de tarefas bloqueadas */
static blocked_task_t *blocked_queue = NULL;
static uint32_t blocked_count = 0;

/* ============================================
   Funções Auxiliares Internas
   ============================================ */

/**
 * @brief Converte milissegundos em ticks do sistema
 */
static uint32_t ms_to_ticks(uint32_t ms) {
    if (ms == 0) {
        return 0;  /* Infinito */
    }
    return (ms * RTOS_TICK_RATE_HZ) / 1000;
}

/**
 * @brief Converte ticks em milissegundos
 */
static uint32_t ticks_to_ms(uint32_t ticks) {
    return (ticks * 1000) / RTOS_TICK_RATE_HZ;
}

/**
 * @brief Encontra tarefa bloqueada por handle
 */
static blocked_task_t *find_blocked_task(rtos_task_handle_t task_handle) {
    blocked_task_t *current = blocked_queue;
    
    while (current != NULL) {
        if (current->task_handle == task_handle) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* ============================================
   API Pública
   ============================================ */

void blocking_block_task(
    rtos_task_handle_t task_handle,
    block_type_t block_type,
    uint32_t resource_id,
    uint32_t timeout_ms
) {
    if (task_handle == 0) {
        return;
    }
    
    /* Verifica se tarefa já está bloqueada */
    blocked_task_t *existing = find_blocked_task(task_handle);
    if (existing != NULL) {
        printf("[RTOS Blocking] AVISO: Tarefa %u já está bloqueada\n", task_handle);
        return;
    }
    
    /* Aloca novo nó de bloqueio */
    blocked_task_t *blocked = (blocked_task_t *)rtos_malloc(sizeof(blocked_task_t));
    if (blocked == NULL) {
        printf("[RTOS Blocking] ERRO: Sem memória para bloqueio\n");
        return;
    }
    
    /* Inicializa nó */
    blocked->task_handle = task_handle;
    blocked->resource_id = resource_id;
    blocked->block_type = block_type;
    blocked->block_time = rtos_kernel_get_ticks();
    blocked->timeout_ticks = ms_to_ticks(timeout_ms);
    blocked->next = NULL;
    
    /* Adiciona à fila de bloqueados (ao final) */
    if (blocked_queue == NULL) {
        blocked_queue = blocked;
    } else {
        blocked_task_t *current = blocked_queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = blocked;
    }
    
    blocked_count++;
    
    /* Atualiza estado da tarefa */
    rtos_task_t *task = scheduler_get_task_by_handle(task_handle);
    if (task != NULL) {
        task->state = RTOS_TASK_BLOCKED;
        task->block_info.blocked_on = resource_id;
        if (timeout_ms != 0) {
            task->block_info.timeout = blocked->block_time + blocked->timeout_ticks;
        }
    }
    
    printf("[RTOS Blocking] Tarefa %u bloqueada (tipo: %d, timeout: %u ms)\n",
           task_handle, block_type, timeout_ms);
}

void blocking_unblock_task(rtos_task_handle_t task_handle) {
    if (task_handle == 0 || blocked_queue == NULL) {
        return;
    }
    
    /* Encontra e remove da fila */
    blocked_task_t *current = blocked_queue;
    blocked_task_t *prev = NULL;
    
    while (current != NULL) {
        if (current->task_handle == task_handle) {
            /* Remove da fila */
            if (prev == NULL) {
                blocked_queue = current->next;
            } else {
                prev->next = current->next;
            }
            
            blocked_count--;
            
            /* Atualiza estado da tarefa */
            rtos_task_t *task = scheduler_get_task_by_handle(task_handle);
            if (task != NULL) {
                task->state = RTOS_TASK_READY;
                task->block_info.blocked_on = 0;
                task->block_info.timeout = 0;
            }
            
            printf("[RTOS Blocking] Tarefa %u desbloqueada\n", task_handle);
            
            /* Libera memória */
            rtos_free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void blocking_process_timeouts(void) {
    uint32_t current_ticks = rtos_kernel_get_ticks();
    blocked_task_t *current = blocked_queue;
    blocked_task_t *prev = NULL;
    
    while (current != NULL) {
        blocked_task_t *next = current->next;  /* Salva próximo antes de deletar */
        
        /* Se timeout é infinito (0), pula */
        if (current->timeout_ticks == 0) {
            prev = current;
            current = next;
            continue;
        }
        
        /* Calcula tempo decorrido */
        uint32_t elapsed = current_ticks - current->block_time;
        
        /* Se timeout expirou */
        if (elapsed >= current->timeout_ticks) {
            printf("[RTOS Blocking] TIMEOUT: Tarefa %u (elapsed: %u ticks, timeout: %u)\n",
                   current->task_handle, elapsed, current->timeout_ticks);
            
            /* Atualiza estado da tarefa */
            rtos_task_t *task = scheduler_get_task_by_handle(current->task_handle);
            if (task != NULL) {
                task->state = RTOS_TASK_READY;
                task->block_info.blocked_on = 0;
                task->block_info.timeout = 0;
            }
            
            /* Remove da fila de bloqueados */
            if (prev == NULL) {
                blocked_queue = next;
            } else {
                prev->next = next;
            }
            
            blocked_count--;
            
            /* Libera memória */
            rtos_free(current);
        } else {
            prev = current;
        }
        
        current = next;
    }
}

uint32_t blocking_get_blocked_count(void) {
    return blocked_count;
}

bool blocking_is_task_blocked(
    rtos_task_handle_t task_handle,
    block_type_t *block_type,
    uint32_t *resource_id
) {
    blocked_task_t *blocked = find_blocked_task(task_handle);
    
    if (blocked == NULL) {
        return false;
    }
    
    if (block_type != NULL) {
        *block_type = blocked->block_type;
    }
    if (resource_id != NULL) {
        *resource_id = blocked->resource_id;
    }
    
    return true;
}

/* Importar função do scheduler */
extern rtos_task_t *scheduler_get_task_by_handle(rtos_task_handle_t handle);
