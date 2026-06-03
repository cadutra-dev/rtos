#ifndef TASK_H
#define TASK_H

#include "../../include/rtos.h"
#include <stdint.h>
#include <stdbool.h>

#define TASK_NAME_MAX_LEN 32
#define TASK_STACK_ALIGN  8

/* Estrutura interna da tarefa */
typedef struct {
    rtos_task_handle_t id;              /* ID único da tarefa */
    char name[TASK_NAME_MAX_LEN];       /* Nome da tarefa */
    rtos_task_function_t entry_point;   /* Função de entrada */
    void *param;                        /* Parâmetro da função */
    uint8_t *stack;                     /* Stack da tarefa */
    size_t stack_size;                  /* Tamanho da stack */
    uint8_t priority;                   /* Prioridade (0-31) */
    rtos_task_state_t state;            /* Estado atual */
    uint32_t sleep_until;               /* Tempo até acordar (ticks) */
    void *sp;                           /* Stack pointer (para contexto) */
    struct {
        uint32_t blocked_on;            /* Bloqueado em que recurso? */
        uint32_t timeout;               /* Timeout do bloqueio */
    } block_info;
} rtos_task_t;

/* Funções de tarefa */
rtos_task_t *task_create(
    rtos_task_function_t func,
    const char *name,
    size_t stack_size,
    void *param,
    uint8_t priority
);

void task_delete(rtos_task_t *task);
void task_set_state(rtos_task_t *task, rtos_task_state_t state);
rtos_task_state_t task_get_state(rtos_task_t *task);

#endif /* TASK_H */
