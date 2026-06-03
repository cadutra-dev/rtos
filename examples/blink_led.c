#include "../include/rtos.h"
#include <stdio.h>
#include <stdbool.h>

/* Pino do LED */
#define LED_PIN 2

volatile bool led_state = false;

void blink_task(void *param) {
    (void)param;

    while (1) {
        /* Pisca LED */
        led_state = !led_state;
        printf("LED: %s\n", led_state ? "ON" : "OFF");

        /* Espera 500ms */
        rtos_task_sleep(500);
    }
}

void monitor_task(void *param) {
    (void)param;

    while (1) {
        printf("[Monitor] Sistema em execução - Ticks: %u\n", 
               rtos_kernel_get_ticks());

        /* Espera 2 segundos */
        rtos_task_sleep(2000);
    }
}

void app_main(void) {
    printf("\n=== RTOS Blink LED Example ===");
    printf("\nInitializing RTOS...\n");

    /* Inicializa kernel */
    if (rtos_kernel_init() != RTOS_OK) {
        printf("ERROR: Failed to initialize RTOS\n");
        return;
    }

    /* Cria tarefa de piscar LED */
    rtos_task_handle_t blink_handle = rtos_task_create(
        blink_task,
        "BlinkTask",
        2048,
        NULL,
        10  /* Prioridade média */
    );

    if (blink_handle == 0) {
        printf("ERROR: Failed to create blink task\n");
        return;
    }

    /* Cria tarefa de monitoramento */
    rtos_task_handle_t monitor_handle = rtos_task_create(
        monitor_task,
        "MonitorTask",
        2048,
        NULL,
        5  /* Prioridade mais baixa */
    );

    if (monitor_handle == 0) {
        printf("ERROR: Failed to create monitor task\n");
        return;
    }

    printf("\nRTOS tasks created. Starting kernel...\n\n");

    /* Inicia scheduler */
    rtos_kernel_start();
}
