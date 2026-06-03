#include "../../include/rtos.h"
#include <stdio.h>

/* Handler padrão para interrupções */
void rtos_isr_handler(void) {
    rtos_kernel_tick();
}
