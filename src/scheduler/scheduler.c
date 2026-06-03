#include "scheduler.h"
#include "task.h"
#include "../../include/rtos.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Estrutura interna do scheduler */
typedef struct {
    rtos_task_t *tasks[RTOS_MAX_TASKS];
    uint32_t task_count;
    uint32_t current_index;
} scheduler_t;

static scheduler_t scheduler = {0};

void scheduler_init(void) {
    memset(&scheduler, 0, sizeof(scheduler_t));
    scheduler.current_index = 0;
}

rtos_task_handle_t scheduler_add_task(
    rtos_task_function_t func,
    const char *name,
    size_t stack_size,
    void *param,
    uint8_t priority
) {
    if (scheduler.task_count >= RTOS_MAX_TASKS) {
        return 0;  /* Limite de tarefas atingido */
    }

    rtos_task_t *task = task_create(func, name, stack_size, param, priority);
    if (task == NULL) {
        return 0;
    }

    scheduler.tasks[scheduler.task_count] = task;
    scheduler.task_count++;

    return task->id;
}

void scheduler_remove_task(rtos_task_handle_t handle) {
    for (uint32_t i = 0; i < scheduler.task_count; i++) {
        if (scheduler.tasks[i] != NULL && scheduler.tasks[i]->id == handle) {
            task_delete(scheduler.tasks[i]);
            scheduler.tasks[i] = NULL;

            /* Remove do array */
            for (uint32_t j = i; j < scheduler.task_count - 1; j++) {
                scheduler.tasks[j] = scheduler.tasks[j + 1];
            }
            scheduler.task_count--;
            return;
        }
    }
}

rtos_task_t *scheduler_get_next_task(void) {
    if (scheduler.task_count == 0) {
        return NULL;
    }

    /* Procura próxima tarefa pronta com maior prioridade */
    rtos_task_t *best_task = NULL;
    int best_priority = -1;
    uint32_t best_index = 0;

    for (uint32_t i = 0; i < scheduler.task_count; i++) {
        rtos_task_t *task = scheduler.tasks[i];

        if (task == NULL) {
            continue;
        }

        if (task->state == RTOS_TASK_READY && task->priority > best_priority) {
            best_task = task;
            best_priority = task->priority;
            best_index = i;
        }
    }

    if (best_task != NULL) {
        scheduler.current_index = best_index;
        best_task->state = RTOS_TASK_RUNNING;
    }

    return best_task;
}

void scheduler_update_tasks(void) {
    uint32_t current_ticks = rtos_kernel_get_ticks();

    for (uint32_t i = 0; i < scheduler.task_count; i++) {
        rtos_task_t *task = scheduler.tasks[i];

        if (task == NULL) {
            continue;
        }

        /* Acordar tarefas que terminaram sleep */
        if (task->state == RTOS_TASK_SLEEPING) {
            if (current_ticks >= task->sleep_until) {
                task->state = RTOS_TASK_READY;
            }
        }

        /* Verificar timeout de bloqueios */
        if (task->state == RTOS_TASK_BLOCKED) {
            if (task->block_info.timeout > 0) {
                if (current_ticks >= task->block_info.timeout) {
                    task->state = RTOS_TASK_READY;
                    task->block_info.blocked_on = 0;
                }
            }
        }
    }
}

rtos_task_t *scheduler_get_task_by_handle(rtos_task_handle_t handle) {
    for (uint32_t i = 0; i < scheduler.task_count; i++) {
        if (scheduler.tasks[i] != NULL && scheduler.tasks[i]->id == handle) {
            return scheduler.tasks[i];
        }
    }
    return NULL;
}
