#ifndef SYNC_BLOCKING_H
#define SYNC_BLOCKING_H

#include "../../include/rtos.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================
   Constantes de Timeout
   ============================================ */

#define RTOS_WAIT_FOREVER       0           /* Aguarda indefinidamente */
#define RTOS_NO_WAIT            1           /* Não aguarda (não bloqueia) */

/* ============================================
   Estados de Bloqueio
   ============================================ */

typedef enum {
    BLOCK_NONE = 0,                        /* Não bloqueado */
    BLOCK_SEMAPHORE,                       /* Bloqueado em semáforo */
    BLOCK_MUTEX,                           /* Bloqueado em mutex */
    BLOCK_QUEUE_SEND,                      /* Bloqueado em fila (envio) */
    BLOCK_QUEUE_RECEIVE                    /* Bloqueado em fila (recepção) */
} block_type_t;

/* ============================================
   Fila de Bloqueio
   ============================================ */

typedef struct blocked_task {
    rtos_task_handle_t task_handle;        /* Tarefa bloqueada */
    uint32_t resource_id;                  /* ID do recurso (sem, mutex, fila) */
    block_type_t block_type;               /* Tipo de bloqueio */
    uint32_t timeout_ticks;                /* Timeout em ticks do sistema */
    uint32_t block_time;                   /* Tempo em que foi bloqueado */
    struct blocked_task *next;             /* Próximo na fila */
} blocked_task_t;

/* ============================================
   API de Bloqueio
   ============================================ */

/**
 * @brief Bloqueia uma tarefa em um recurso com timeout
 * @param task_handle Handle da tarefa
 * @param block_type Tipo de bloqueio
 * @param resource_id ID do recurso
 * @param timeout_ms Timeout em milissegundos (0 = infinito)
 */
void blocking_block_task(
    rtos_task_handle_t task_handle,
    block_type_t block_type,
    uint32_t resource_id,
    uint32_t timeout_ms
);

/**
 * @brief Desbloqueia uma tarefa
 * @param task_handle Handle da tarefa
 */
void blocking_unblock_task(rtos_task_handle_t task_handle);

/**
 * @brief Verifica e processa timeouts de bloqueio
 * Chamado pelo scheduler a cada tick
 */
void blocking_process_timeouts(void);

/**
 * @brief Obtém número de tarefas bloqueadas
 * @return Número de tarefas bloqueadas
 */
uint32_t blocking_get_blocked_count(void);

/**
 * @brief Obtém informações de bloqueio de uma tarefa
 * @param task_handle Handle da tarefa
 * @param block_type Ponteiro para tipo de bloqueio
 * @param resource_id Ponteiro para ID do recurso
 * @return true se tarefa está bloqueada
 */
bool blocking_is_task_blocked(
    rtos_task_handle_t task_handle,
    block_type_t *block_type,
    uint32_t *resource_id
);

#endif /* SYNC_BLOCKING_H */
