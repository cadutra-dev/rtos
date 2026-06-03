#ifndef BLOCKED_QUEUE_H
#define BLOCKED_QUEUE_H

#include "../../include/rtos.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================
   Modos de Ordenação de Fila
   ============================================ */

typedef enum {
    BLOCKED_QUEUE_FIFO,         /* Primeiro chegou, primeiro sai */
    BLOCKED_QUEUE_PRIORITY,     /* Ordenado por prioridade */
    BLOCKED_QUEUE_TIMEOUT       /* Ordenado por tempo de timeout */
} blocked_queue_order_t;

/* ============================================
   Nó da Fila de Bloqueio
   ============================================ */

typedef struct blocked_queue_node {
    rtos_task_handle_t task_handle;     /* ID da tarefa */
    uint32_t resource_id;               /* ID do recurso */
    block_type_t block_type;            /* Tipo de bloqueio */
    uint32_t timeout_ticks;             /* Timeout em ticks */
    uint32_t block_time;                /* Quando foi bloqueado */
    uint8_t priority;                   /* Prioridade da tarefa */
    uint32_t wait_time;                 /* Tempo já esperado */
    struct blocked_queue_node *prev;    /* Nó anterior */
    struct blocked_queue_node *next;    /* Próximo nó */
} blocked_queue_node_t;

/* ============================================
   Estrutura de Fila de Bloqueio
   ============================================ */

typedef struct {
    blocked_queue_node_t *head;         /* Primeiro elemento */
    blocked_queue_node_t *tail;         /* Último elemento */
    uint32_t count;                     /* Número de elementos */
    blocked_queue_order_t order;        /* Modo de ordenação */
    uint32_t max_size;                  /* Tamanho máximo */
} blocked_queue_t;

/* ============================================
   Estatísticas
   ============================================ */

typedef struct {
    uint32_t total_blocked;             /* Total que passaram por bloqueio */
    uint32_t current_blocked;           /* Bloqueados agora */
    uint32_t max_blocked;               /* Pico de bloqueados */
    uint32_t timeouts;                  /* Total de timeouts */
    uint32_t avg_wait_time;             /* Tempo médio de espera */
    uint32_t max_wait_time;             /* Tempo máximo de espera */
} blocked_queue_stats_t;

/* ============================================
   API de Fila de Bloqueio
   ============================================ */

/**
 * @brief Inicializa a fila de bloqueio global
 * @param order Modo de ordenação
 * @param max_size Tamanho máximo (0 = ilimitado)
 * @return RTOS_OK em caso de sucesso
 */
int blocked_queue_init(blocked_queue_order_t order, uint32_t max_size);

/**
 * @brief Adiciona tarefa à fila de bloqueio
 * @param task_handle Handle da tarefa
 * @param block_type Tipo de bloqueio
 * @param resource_id ID do recurso
 * @param timeout_ms Timeout em ms (0 = infinito)
 * @return RTOS_OK se sucesso
 */
int blocked_queue_add(
    rtos_task_handle_t task_handle,
    block_type_t block_type,
    uint32_t resource_id,
    uint32_t timeout_ms
);

/**
 * @brief Remove tarefa da fila de bloqueio
 * @param task_handle Handle da tarefa
 * @return Nó removido ou NULL
 */
blocked_queue_node_t *blocked_queue_remove(rtos_task_handle_t task_handle);

/**
 * @brief Remove primeiro da fila (mais antigo)
 * @return Nó removido ou NULL
 */
blocked_queue_node_t *blocked_queue_remove_first(void);

/**
 * @brief Remove primeiro com prioridade mais alta
 * @return Nó removido ou NULL
 */
blocked_queue_node_t *blocked_queue_remove_highest_priority(void);

/**
 * @brief Processa tarefas com timeout expirado
 * @return Número de tarefas desbloqueadas por timeout
 */
uint32_t blocked_queue_process_timeouts(void);

/**
 * @brief Encontra tarefa bloqueada
 * @param task_handle Handle da tarefa
 * @return Nó encontrado ou NULL
 */
blocked_queue_node_t *blocked_queue_find(rtos_task_handle_t task_handle);

/**
 * @brief Obtém primeira tarefa que pode ser desbloqueada
 * @return Nó com timeout próximo ou NULL
 */
blocked_queue_node_t *blocked_queue_get_next_timeout(void);

/**
 * @brief Obtém número de tarefas bloqueadas
 * @return Número de tarefas
 */
uint32_t blocked_queue_get_count(void);

/**
 * @brief Obtém tamanho máximo da fila
 * @return Tamanho máximo (0 = ilimitado)
 */
uint32_t blocked_queue_get_max_size(void);

/**
 * @brief Verifica se fila está cheia
 * @return true se cheia
 */
bool blocked_queue_is_full(void);

/**
 * @brief Verifica se fila está vazia
 * @return true se vazia
 */
bool blocked_queue_is_empty(void);

/**
 * @brief Limpa toda a fila
 */
void blocked_queue_clear(void);

/**
 * @brief Obtém estatísticas
 * @param stats Ponteiro para estrutura a preencher
 */
void blocked_queue_get_stats(blocked_queue_stats_t *stats);

/**
 * @brief Imprime conteúdo da fila (debug)
 */
void blocked_queue_print_queue(void);

/**
 * @brief Valida integridade da fila
 * @return true se válida
 */
bool blocked_queue_validate(void);

/**
 * @brief Muda modo de ordenação (reordena)
 * @param new_order Novo modo de ordenação
 * @return RTOS_OK se sucesso
 */
int blocked_queue_set_order(blocked_queue_order_t new_order);

#endif /* BLOCKED_QUEUE_H */
