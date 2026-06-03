#include "scheduler.h"
#include "../../include/rtos.h"
#include <stdio.h>

rtos_task_handle_t rtos_task_create(
    rtos_task_function_t func,
    const char *name,
    size_t stack_size,
    void *param,
    uint8_t priority
) {
    return scheduler_add_task(func, name, stack_size, param, priority);
}

int rtos_task_delete(rtos_task_handle_t task_handle) {
    scheduler_remove_task(task_handle);
    return RTOS_OK;
}

void rtos_task_sleep(uint32_t ms) {
    uint32_t ticks = (ms * RTOS_TICK_RATE_HZ) / 1000;
    uint32_t current_ticks = rtos_kernel_get_ticks();

    rtos_task_handle_t current = rtos_task_get_current();
    rtos_task_t *task = scheduler_get_task_by_handle(current);

    if (task != NULL) {
        task->state = RTOS_TASK_SLEEPING;
        task->sleep_until = current_ticks + ticks;
    }

    /* Cede processador */
    rtos_task_yield();
}

void rtos_task_yield(void) {
    /* Marca tarefa como pronta */
    rtos_task_handle_t current = rtos_task_get_current();
    rtos_task_t *task = scheduler_get_task_by_handle(current);

    if (task != NULL) {
        task->state = RTOS_TASK_READY;
    }

    /* Força context switch */
    /* (A implementação real dependeria de interrupção de timer) */
}

uint8_t rtos_task_get_priority(rtos_task_handle_t task_handle) {
    rtos_task_t *task = scheduler_get_task_by_handle(task_handle);
    if (task != NULL) {
        return task->priority;
    }
    return 0;
}

int rtos_task_set_priority(rtos_task_handle_t task_handle, uint8_t priority) {
    if (priority > 31) {
        return RTOS_INVALID_PARAM;
    }

    rtos_task_t *task = scheduler_get_task_by_handle(task_handle);
    if (task != NULL) {
        task->priority = priority;
        return RTOS_OK;
    }

    return RTOS_ERROR;
}

rtos_task_state_t rtos_task_get_state(rtos_task_handle_t task_handle) {
    rtos_task_t *task = scheduler_get_task_by_handle(task_handle);
    if (task != NULL) {
        return task->state;
    }
    return RTOS_TASK_TERMINATED;
}

rtos_task_handle_t rtos_task_get_current(void) {
    extern rtos_kernel_t *kernel_get_instance(void);
    rtos_kernel_t *kernel = kernel_get_instance();
    if (kernel->current_task != NULL) {
        return kernel->current_task->id;
    }
    return 0;
}
