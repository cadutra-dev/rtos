#include "task.h"
#include "../../include/rtos.h"
#include "../platform/esp32/context.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uint32_t next_task_id = 1;

rtos_task_t *task_create(
    rtos_task_function_t func,
    const char *name,
    size_t stack_size,
    void *param,
    uint8_t priority
) {
    if (func == NULL || stack_size == 0 || priority > 31) {
        return NULL;
    }

    /* Aloca estrutura da tarefa */
    rtos_task_t *task = (rtos_task_t *)rtos_malloc(sizeof(rtos_task_t));
    if (task == NULL) {
        return NULL;
    }

    /* Aloca stack da tarefa */
    task->stack = (uint8_t *)rtos_malloc(stack_size);
    if (task->stack == NULL) {
        rtos_free(task);
        return NULL;
    }

    /* Inicializa estrutura */
    task->id = next_task_id++;
    strncpy(task->name, name, TASK_NAME_MAX_LEN - 1);
    task->name[TASK_NAME_MAX_LEN - 1] = '\0';
    task->entry_point = func;
    task->param = param;
    task->stack_size = stack_size;
    task->priority = priority;
    task->state = RTOS_TASK_READY;
    task->sleep_until = 0;
    task->block_info.blocked_on = 0;
    task->block_info.timeout = 0;

    /* Inicializa stack com contexto válido */
    context_init_stack(task);

    return task;
}

void task_delete(rtos_task_t *task) {
    if (task == NULL) {
        return;
    }

    if (task->stack != NULL) {
        rtos_free(task->stack);
    }

    rtos_free(task);
}

void task_set_state(rtos_task_t *task, rtos_task_state_t state) {
    if (task != NULL) {
        task->state = state;
    }
}

rtos_task_state_t task_get_state(rtos_task_t *task) {
    if (task == NULL) {
        return RTOS_TASK_TERMINATED;
    }
    return task->state;
}
