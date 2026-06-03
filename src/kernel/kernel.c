#include "../../include/rtos.h"
#include "../kernel/kernel_internal.h"
#include "../sync/blocking.h"
#include <string.h>
#include <stdio.h>

/* Variáveis globais do kernel */
static rtos_kernel_t kernel = {0};
static bool kernel_initialized = false;
static bool kernel_running = false;

/* ============================================
   Funções Internas
   ============================================ */

static void kernel_idle_task(void *param) {
    (void)param;
    while (1) {
        /* Task idle apenas cede processamento */
        rtos_task_yield();
    }
}

/* ============================================
   API Pública
   ============================================ */

int rtos_kernel_init(void) {
    if (kernel_initialized) {
        return RTOS_ERROR;
    }

    /* Inicializa estrutura do kernel */
    memset(&kernel, 0, sizeof(rtos_kernel_t));
    kernel.ticks = 0;
    kernel.current_task = NULL;

    /* Inicializa scheduler */
    scheduler_init();

    /* Inicializa gerenciador de memória */
    memory_init();

    /* Cria tarefa idle */
    kernel.idle_task = rtos_task_create(
        kernel_idle_task,
        "IDLE",
        512,
        NULL,
        0  /* Menor prioridade possível */
    );

    if (kernel.idle_task == 0) {
        return RTOS_ERROR;
    }

    kernel_initialized = true;
    printf("[RTOS] Kernel initialized\n");
    return RTOS_OK;
}

int rtos_kernel_start(void) {
    if (!kernel_initialized) {
        return RTOS_ERROR;
    }

    kernel_running = true;

    /* Inicia timer de tick */
    timer_init();

    printf("[RTOS] Kernel starting scheduler...\n");

    /* Seleciona primeira tarefa pronta */
    kernel.current_task = scheduler_get_next_task();

    if (kernel.current_task == NULL) {
        return RTOS_ERROR;
    }

    printf("[RTOS] Starting task: %s\n", kernel.current_task->name);

    /* Restaura contexto da primeira tarefa */
    /* Esta chamada nunca retorna - restaura o contexto completamente */
    context_restore(kernel.current_task);

    /* Nunca chega aqui */
    return RTOS_OK;
}

void rtos_kernel_tick(void) {
    if (!kernel_running) {
        return;
    }

    kernel.ticks++;

    /* Processa timeouts de bloqueio */
    blocking_process_timeouts();

    /* Atualiza scheduler */
    scheduler_update_tasks();

    /* Obtém próxima tarefa pronta */
    rtos_task_t *next_task = scheduler_get_next_task();

    if (next_task == NULL) {
        return;
    }

    /* Realiza context switch se necessário */
    if (next_task != kernel.current_task) {
        rtos_task_t *prev_task = kernel.current_task;
        kernel.current_task = next_task;

        /* Salva contexto da tarefa anterior */
        if (prev_task != NULL) {
            context_save(prev_task);
        }

        /* Restaura contexto da próxima tarefa */
        /* Esta chamada nunca retorna - restaura o contexto completamente */
        context_restore(next_task);
    }
}

uint32_t rtos_kernel_get_ticks(void) {
    return kernel.ticks;
}

/* Funções de acesso interno */

rtos_kernel_t *kernel_get_instance(void) {
    return &kernel;
}

bool kernel_is_running(void) {
    return kernel_running;
}
