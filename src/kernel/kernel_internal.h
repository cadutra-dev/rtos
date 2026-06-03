#ifndef KERNEL_INTERNAL_H
#define KERNEL_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "../scheduler/task.h"

typedef struct {
    uint32_t ticks;                     /* Contador de ticks do sistema */
    rtos_task_t *current_task;          /* Tarefa em execução */
    rtos_task_handle_t idle_task;       /* Handle da tarefa idle */
} rtos_kernel_t;

/* Funções internas */
rtos_kernel_t *kernel_get_instance(void);
bool kernel_is_running(void);

/* Funções do scheduler */
void scheduler_init(void);
void scheduler_update_tasks(void);
rtos_task_t *scheduler_get_next_task(void);

/* Funções de memória */
void memory_init(void);

/* Funções de contexto */
void context_save(rtos_task_t *task);
void context_restore(rtos_task_t *task);

/* Funções de timer */
void timer_init(void);

#endif /* KERNEL_INTERNAL_H */
