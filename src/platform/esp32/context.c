#include "../../../include/rtos.h"
#include "../../../src/scheduler/task.h"
#include <stdio.h>

/* Stubs para salvar/restaurar contexto */

void context_save(rtos_task_t *task) {
    if (task == NULL) return;
    /* TODO: Salvar registradores e estado da CPU */
}

void context_restore(rtos_task_t *task) {
    if (task == NULL) return;
    /* TODO: Restaurar registradores e estado da CPU */
}
