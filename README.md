# RTOS - Real-Time Operating System for ESP Microcontroller

Um sistema operacional em tempo real (RTOS) leve e eficiente desenvolvido em C para microcontroladores ESP (ESP32, ESP8266).

## Características

- ✅ Kernel preemptivo com scheduler Round-Robin
- ✅ Gerenciamento de tarefas (tasks/threads)
- ✅ Sincronização primitiva (semáforos, mutexes)
- ✅ Fila de mensagens (message queue)
- ✅ Gerenciamento de memória
- ✅ Suporte para ESP32
- ✅ Contexto de interrupção seguro

## Estrutura do Projeto

```
rtos/
├── src/
│   ├── kernel/          # Core do kernel
│   ├── scheduler/       # Scheduler e gerenciador de tarefas
│   ├── sync/            # Primitivos de sincronização
│   ├── memory/          # Gerenciador de memória
│   ├── isr/             # Tratamento de interrupções
│   └── platform/        # Código específico da plataforma (ESP32)
├── include/             # Headers públicos
├── examples/            # Exemplos de uso
├── tests/               # Testes unitários
├── docs/                # Documentação
└── CMakeLists.txt       # Configuração build
```

## Começando

### Pré-requisitos

- ESP-IDF (ESP IoT Development Framework)
- GCC para Xtensa (incluso no ESP-IDF)
- CMake

### Compilação

```bash
cd rtos
idf.py build
```

### Upload para ESP32

```bash
idf.py flash monitor
```

## Exemplo Básico

```c
#include "rtos.h"

void task1(void *param) {
    while (1) {
        printf("Task 1 executando\n");
        rtos_task_sleep(1000); // 1 segundo
    }
}

void task2(void *param) {
    while (1) {
        printf("Task 2 executando\n");
        rtos_task_sleep(2000); // 2 segundos
    }
}

void app_main() {
    rtos_kernel_init();
    
    rtos_task_create(task1, "Task1", 2048, NULL, 5);
    rtos_task_create(task2, "Task2", 2048, NULL, 4);
    
    rtos_kernel_start();
}
```

## API Principal

### Kernel
- `rtos_kernel_init()` - Inicializa o kernel
- `rtos_kernel_start()` - Inicia o scheduler
- `rtos_kernel_tick()` - Tick do kernel (chamado por timer)

### Tarefas
- `rtos_task_create()` - Cria uma nova tarefa
- `rtos_task_delete()` - Deleta uma tarefa
- `rtos_task_sleep()` - Coloca tarefa em sleep
- `rtos_task_yield()` - Cede o processador
- `rtos_task_get_priority()` - Obtém prioridade
- `rtos_task_set_priority()` - Define prioridade

### Sincronização
- `rtos_semaphore_create()` - Cria semáforo
- `rtos_semaphore_take()` - Aguarda semáforo
- `rtos_semaphore_give()` - Libera semáforo
- `rtos_mutex_create()` - Cria mutex
- `rtos_mutex_lock()` - Bloqueia mutex
- `rtos_mutex_unlock()` - Desbloqueia mutex

### Fila de Mensagens
- `rtos_queue_create()` - Cria fila
- `rtos_queue_send()` - Envia mensagem
- `rtos_queue_receive()` - Recebe mensagem

## Documentação

Veja a pasta `docs/` para documentação detalhada sobre a arquitetura e implementação.

## Exemplos

Exemplos práticos estão em `examples/`:
- `blink_led.c` - Piscar LED
- `multi_task.c` - Múltiplas tarefas
- `semaphore_example.c` - Uso de semáforos
- `uart_comm.c` - Comunicação UART

## Licença

MIT

## Autor

cadutra-dev
