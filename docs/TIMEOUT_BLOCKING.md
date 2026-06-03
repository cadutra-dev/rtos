# Timeout em Bloqueios - Sistema Completo

## Visão Geral

Implementação de um sistema completo de **timeout em bloqueios** para sincronização RTOS, permitindo:

- **Timeout em Semáforos** - Aguarda com limite de tempo
- **Timeout em Mutexes** - Lock com deadline
- **Timeout em Filas** - Send/Receive com timeout
- **Detecção automática** de timeouts expirados
- **Gestão de tarefas bloqueadas** com fila de prioridade
- **Processamento eficiente** a cada tick do kernel

## Arquitetura

### Componentes Principais

```
┌─────────────────────────────────────────┐
│      Tarefas Prontas (Scheduler)       │
└─────────────────────────────────────────┘
           │                  │
           │                  ├─→ RTOS_TASK_READY
           │                  ├─→ RTOS_TASK_RUNNING
           │                  └─→ RTOS_TASK_IDLE
           │
           └─→ Bloqueadas?
                   │
        ┌──────────┴──────────┬──────────┬──────────┐
        │                     │          │          │
   Semáforo              Mutex       Fila-Send  Fila-Rcv
        │                     │          │          │
    Aguardando             Aguardando  Aguardando Aguardando
     Recurso                Lock       Espaço     Item
        │                     │          │          │
        └──────────┬──────────┴──────────┴──────────┘
                   │
        ┌──────────▼──────────┐
        │  Fila de Bloqueados │
        │ (blocking_queue_t)  │
        └─────────────────────┘
                   │
     ┌─────────────┴─────────────┐
     │ A cada rtos_kernel_tick() │
     └─────────────┬─────────────┘
                   │
        ┌──────────▼──────────┐
        │ Verifica Timeouts   │
        │ blocking_process_   │
        │ timeouts()          │
        └─────────────────────┘
```

### Estrutura de Bloqueio

```c
typedef struct blocked_task {
    rtos_task_handle_t task_handle;     /* ID da tarefa */
    uint32_t resource_id;              /* ID do recurso */
    block_type_t block_type;           /* Tipo de bloqueio */
    uint32_t timeout_ticks;            /* Timeout em ticks */
    uint32_t block_time;               /* Quando foi bloqueado */
    struct blocked_task *next;         /* Próximo na fila */
} blocked_task_t;
```

## Estados de Bloqueio

```c
typedef enum {
    BLOCK_NONE = 0,
    BLOCK_SEMAPHORE,        /* Esperando semáforo */
    BLOCK_MUTEX,            /* Esperando mutex */
    BLOCK_QUEUE_SEND,       /* Esperando espaço em fila */
    BLOCK_QUEUE_RECEIVE     /* Esperando item em fila */
} block_type_t;
```

## Fluxo de Operação

### 1. Bloqueio com Timeout

```
Tarefa chama:
├─ rtos_semaphore_take(sem, 1000)  // timeout 1000ms
│
├─ Se recurso disponível → Toma imediatamente (RTOS_OK)
│
├─ Senão:
│  ├─ blocking_block_task()
│  │  └─ Adiciona à fila de bloqueados
│  ├─ rtos_task_yield()
│  │  └─ Context switch para próxima tarefa
│  └─ Retorna quando desbloqueada ou timeout
│
└─ Retorna: RTOS_OK ou RTOS_TIMEOUT
```

### 2. Processamento de Timeouts (A cada 1ms)

```
rtos_kernel_tick():
    │
    ├─ blocking_process_timeouts()  /* Verificar expirados */
    │  ├─ Para cada tarefa bloqueada:
    │  │  ├─ elapsed = current_ticks - block_time
    │  │  ├─ Se elapsed >= timeout_ticks:
    │  │  │  ├─ Mark RTOS_TASK_READY
    │  │  │  ├─ Remove da fila de bloqueados
    │  │  │  └─ Free memória
    │  │  └─ Senão: continua bloqueada
    │  │
    │  └─ Retorna número de tarefas desbloqueadas
    │
    ├─ scheduler_update_tasks()
    ├─ scheduler_get_next_task()
    └─ context_switch()
```

### 3. Desbloqueio Normal (Recurso Disponível)

```
Tarefa detentora libera recurso:
├─ rtos_semaphore_give(sem)
│
├─ Incrementa contador
│
├─ Se há tarefas bloqueadas esperando:
│  ├─ blocking_unblock_task(blocked_task->handle)
│  ├─ Remove de blocked_queue
│  ├─ Mark RTOS_TASK_READY
│  └─ Próximo tick: será selecionada pelo scheduler
│
└─ Retorna RTOS_OK
```

## API Principal

### Bloqueio e Desbloqueio

```c
/* Bloqueia tarefa com timeout */
void blocking_block_task(
    rtos_task_handle_t task_handle,
    block_type_t block_type,
    uint32_t resource_id,
    uint32_t timeout_ms
);

/* Desbloqueia tarefa imediatamente */
void blocking_unblock_task(rtos_task_handle_t task_handle);

/* Processamento periódico de timeouts */
void blocking_process_timeouts(void);
```

### Monitoramento

```c
/* Obtém número de tarefas bloqueadas */
uint32_t blocking_get_blocked_count(void);

/* Verifica se tarefa está bloqueada */
bool blocking_is_task_blocked(
    rtos_task_handle_t task_handle,
    block_type_t *block_type,
    uint32_t *resource_id
);
```

## Constantes Especiais

```c
#define RTOS_WAIT_FOREVER   0       /* Aguarda indefinidamente */
#define RTOS_NO_WAIT        1       /* Não aguarda (não bloqueia) */

/* Exemplos de uso: */
rtos_semaphore_take(sem, RTOS_WAIT_FOREVER);  /* Espera para sempre */
rtos_semaphore_take(sem, RTOS_NO_WAIT);       /* Tenta sem esperar */
rtos_semaphore_take(sem, 1000);               /* Espera até 1 segundo */
```

## Exemplo Prático

### Semáforo com Timeout

```c
rtos_semaphore_t sem = rtos_semaphore_create(0);

void produtor(void *param) {
    int data = 42;
    printf("Produtor: gerando dados\n");
    sleep(2000);  /* Simula processamento */
    rtos_semaphore_give(sem);
    printf("Produtor: dados prontos\n");
}

void consumidor(void *param) {
    printf("Consumidor: aguardando dados...\n");
    
    int result = rtos_semaphore_take(sem, 3000);  /* 3 segundos */
    
    if (result == RTOS_OK) {
        printf("Consumidor: dados recebidos!\n");
    } else if (result == RTOS_TIMEOUT) {
        printf("Consumidor: TIMEOUT! Nenhum dado recebido.\n");
    }
}

void app_main(void) {
    rtos_kernel_init();
    
    rtos_task_create(produtor, "Prod", 2048, NULL, 10);
    rtos_task_create(consumidor, "Cons", 2048, NULL, 10);
    
    rtos_kernel_start();
}
```

### Mutex com Timeout

```c
rtos_mutex_t mtx = rtos_mutex_create();
int shared_resource = 0;

void task1(void *param) {
    while (1) {
        printf("Task1: tentando lock...\n");
        
        int result = rtos_mutex_lock(mtx, 500);  /* 500ms timeout */
        
        if (result == RTOS_OK) {
            printf("Task1: lock adquirido!\n");
            shared_resource++;
            printf("Task1: recurso = %d\n", shared_resource);
            rtos_mutex_unlock(mtx);
            rtos_task_sleep(1000);
        } else {
            printf("Task1: TIMEOUT aguardando mutex!\n");
        }
    }
}

void task2(void *param) {
    while (1) {
        printf("Task2: tentando lock...\n");
        
        int result = rtos_mutex_lock(mtx, 500);
        
        if (result == RTOS_OK) {
            printf("Task2: lock adquirido!\n");
            shared_resource += 10;
            printf("Task2: recurso = %d\n", shared_resource);
            rtos_mutex_unlock(mtx);
            rtos_task_sleep(1000);
        } else {
            printf("Task2: TIMEOUT!\n");
        }
    }
}
```

### Fila com Timeout

```c
rtos_queue_t fila = rtos_queue_create(sizeof(int), 5);

void envio_task(void *param) {
    for (int i = 0; i < 10; i++) {
        int msg = i * 100;
        
        printf("Enviando: %d\n", msg);
        
        int result = rtos_queue_send(fila, &msg, 2000);
        
        if (result == RTOS_OK) {
            printf("Enviado com sucesso\n");
        } else {
            printf("TIMEOUT: Fila cheia!\n");
        }
        
        rtos_task_sleep(500);
    }
}

void recepcao_task(void *param) {
    int msg;
    
    while (1) {
        printf("Aguardando mensagem...\n");
        
        int result = rtos_queue_receive(fila, &msg, 3000);
        
        if (result == RTOS_OK) {
            printf("Recebido: %d\n", msg);
        } else {
            printf("TIMEOUT: Nenhuma mensagem disponível\n");
        }
    }
}
```

## Performance

### Complexidade de Tempo

| Operação | Tempo | Notas |
|----------|-------|-------|
| block_task() | O(1) | Append na fila |
| unblock_task() | O(n) | Busca linear |
| process_timeouts() | O(n) | Verifica todos |
| is_task_blocked() | O(n) | Busca linear |

onde n = número de tarefas bloqueadas (típico: < 32)

### Overhead

- **Por tarefa bloqueada**: 24 bytes (blocked_task_t)
- **Por tick**: ~1-5 µs para processar timeouts (ESP32 @ 240MHz)

## Proteções Contra Erros

### 1. Deadlock Prevention

```c
/* Detecta tentativa de reentrant lock */
if (mutexes[i].owner == current) {
    printf("AVISO: Deadlock detectado (reentrant lock)\n");
    return RTOS_ERROR;
}
```

### 2. Double-Block Prevention

```c
/* Verifica se tarefa já está bloqueada */
existing = find_blocked_task(task_handle);
if (existing != NULL) {
    printf("AVISO: Tarefa %u já está bloqueada\n", task_handle);
    return;
}
```

### 3. Memory Safety

```c
/* Aloca memória dinamicamente com verificação */
blocked = rtos_malloc(sizeof(blocked_task_t));
if (blocked == NULL) {
    printf("ERRO: Sem memória para bloqueio\n");
    return;
}
```

## Diagrama de Estados

```
                    ┌──────────────┐
                    │    READY     │
                    └──────┬───────┘
                           │
                    Selecionado pelo
                     scheduler
                           │
                           ▼
                    ┌──────────────┐
                    │   RUNNING    │
                    └──────┬───────┘
                           │
              Tenta adquirir recurso
              que não está disponível
                           │
                           ▼
                    ┌──────────────┐
                    │   BLOCKED    │───── Timeout expirado ─────┐
                    │ (esperando)  │                            │
                    └──────┬───────┘                            │
                           │                                    │
              Recurso fica disponível                           │
              (outro desbloqueia)                               │
                           │                                    │
                           ▼                                    ▼
                    ┌──────────────┐                   ┌──────────────┐
                    │    READY     │◄──────────────────│    READY     │
                    │ (novamente)  │    (desbloqueada)│  (timeout)   │
                    └──────────────┘                   └──────────────┘
```

## Monitoramento

```c
/* Debug: listar todas as tarefas bloqueadas */
void print_blocked_tasks(void) {
    uint32_t count = blocking_get_blocked_count();
    printf("\nTarefas bloqueadas: %u\n", count);
    
    for (cada tarefa) {
        block_type_t type;
        uint32_t resource;
        blocking_is_task_blocked(task, &type, &resource);
        
        printf("  Tarefa %u: bloqueada em ", task);
        switch (type) {
            case BLOCK_SEMAPHORE: printf("semáforo %u\n", resource); break;
            case BLOCK_MUTEX: printf("mutex %u\n", resource); break;
            case BLOCK_QUEUE_SEND: printf("fila %u (envio)\n", resource); break;
            case BLOCK_QUEUE_RECEIVE: printf("fila %u (recepção)\n", resource); break;
        }
    }
}
```

## Considerações de Projeto

### ✅ Implementado

- Timeout em todos os primitivos (sem, mutex, fila)
- Processamento automático de timeouts
- Sem polling - baseado em eventos
- Memory efficient
- Thread-safe com RTOS

### 🔮 Melhorias Futuras

- [ ] Fila de bloqueio com prioridades
- [ ] Herança de prioridade (priority inheritance)
- [ ] Timeout adaptativo
- [ ] Estatísticas de contenção
- [ ] Profiling de tempo de espera

## Limitações

1. **Máximo de bloqueios**: Limitado por RTOS_MAX_TASKS (32)
2. **Resolução de timeout**: 1ms (rate do kernel tick)
3. **Sem priority donation**: Pode haver inversão de prioridade
4. **Sem cancellation**: Timeout é a única forma de desbloqueio

## Melhor Prática

```c
/* ✅ BOM: Sempre use timeout em bloqueios */
int result = rtos_mutex_lock(mtx, 1000);
if (result == RTOS_OK) {
    /* Usar recurso */
} else {
    /* Handle timeout gracefully */
}

/* ❌ RUIM: Esperar indefinidamente pode causar deadlock */
rtos_mutex_lock(mtx, RTOS_WAIT_FOREVER);

/* ⚠️ CUIDADO: RTOS_NO_WAIT não bloqueia */
if (rtos_mutex_lock(mtx, RTOS_NO_WAIT) == RTOS_OK) {
    /* Lock adquirido */
} else {
    /* Não bloqueou - tente mais tarde */
}
```
