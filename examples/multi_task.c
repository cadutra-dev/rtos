#include "../include/rtos.h"
#include <stdio.h>

/* IDs das tarefas */
rtos_task_handle_t task1_handle;
rtos_task_handle_t task2_handle;
rtos_task_handle_t task3_handle;

void task_1(void *param) {
    (void)param;
    int counter = 0;

    while (1) {
        printf("[Task1] Contador: %d - Prioridade: %d\n", 
               counter++, rtos_task_get_priority(task1_handle));
        rtos_task_sleep(1000);
    }
}

void task_2(void *param) {
    (void)param;
    int counter = 0;

    while (1) {
        printf("  [Task2] Execução: %d - Prioridade: %d\n", 
               counter++, rtos_task_get_priority(task2_handle));
        rtos_task_sleep(1500);
    }
}

void task_3(void *param) {
    (void)param;
    int counter = 0;

    while (1) {
        printf("    [Task3] Iteração: %d - Prioridade: %d\n", 
               counter++, rtos_task_get_priority(task3_handle));
        rtos_task_sleep(2000);
    }
}

void app_main(void) {
    printf("\n=== RTOS Multiple Tasks Example ===");
    printf("\nInitializing RTOS...\n");

    /* Inicializa kernel */
    if (rtos_kernel_init() != RTOS_OK) {
        printf("ERROR: Failed to initialize RTOS\n");
        return;
    }

    /* Cria tarefas com diferentes prioridades */
    task1_handle = rtos_task_create(task_1, "Task1", 2048, NULL, 15);
    task2_handle = rtos_task_create(task_2, "Task2", 2048, NULL, 10);
    task3_handle = rtos_task_create(task_3, "Task3", 2048, NULL, 5);

    if (task1_handle == 0 || task2_handle == 0 || task3_handle == 0) {
        printf("ERROR: Failed to create tasks\n");
        return;
    }

    printf("\nTasks created with priorities:")
    printf("\n  Task1: priority %d", rtos_task_get_priority(task1_handle));
    printf("\n  Task2: priority %d", rtos_task_get_priority(task2_handle));
    printf("\n  Task3: priority %d\n", rtos_task_get_priority(task3_handle));
    printf("\nStarting RTOS kernel...\n\n");

    /* Inicia scheduler */
    rtos_kernel_start();
}
