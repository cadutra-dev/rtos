#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../../include/rtos.h"
#include "task.h"
#include <stdint.h>

void scheduler_init(void);

rtos_task_handle_t scheduler_add_task(
    rtos_task_function_t func,
    const char *name,
    size_t stack_size,
    void *param,
    uint8_t priority
);

void scheduler_remove_task(rtos_task_handle_t handle);
rtos_task_t *scheduler_get_next_task(void);
void scheduler_update_tasks(void);
rtos_task_t *scheduler_get_task_by_handle(rtos_task_handle_t handle);

#endif /* SCHEDULER_H */
