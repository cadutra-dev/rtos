# Fila de Bloqueio (Blocked Queue) - Sistema Avançado

## Visão Geral

Implementação de uma **fila de bloqueio (blocked queue) dinamicamente ordenável** para gerenciamento eficiente de tarefas aguardando recursos.

Características principais:
- ✅ **3 modos de ordenação** (FIFO, Prioridade, Timeout)
- ✅ **Operações O(1) e O(n)** otimizadas
- ✅ **Reordenação dinâmica** sem desalocar
- ✅ **Estatísticas completas** em tempo real
- ✅ **Validação de integridade** automática
- ✅ **Debug avançado** com impressão de fila

## Arquitetura

### Estrutura de Nó

```c
typedef struct blocked_queue_node {
    rtos_task_handle_t task_handle;     /* ID da tarefa */
    uint32_t resource_id;               /* ID do recurso */
    block_type_t block_type;            /* Tipo de bloqueio */
    uint32_t timeout_ticks;             /* Timeout em ticks */
    uint32_t block_time;                /* Quando foi bloqueado */
    uint8_t priority;                   /* Prioridade da tarefa */
    uint32_t wait_time;                 /* Tempo já esperado */
    struct blocked_queue_node *prev;    /* Nó anterior */
    struct blocked_queue_node *next;    /* Próximo nó */
} blocked_queue_node_t;  /* 56 bytes */
```

### Fila de Bloqueio

```c
typedef struct {
    blocked_queue_node_t *head;         /* Primeiro elemento */
    blocked_queue_node_t *tail;         /* Último elemento */
    uint32_t count;                     /* Número de elementos */
    blocked_queue_order_t order;        /* Modo de ordenação */
    uint32_t max_size;                  /* Tamanho máximo */
} blocked_queue_t;
```

## Modos de Ordenação

### 1. FIFO (First In First Out)

```
Adiciona no final, remove do início

Head ─→ [Task 1] ─→ [Task 2] ─→ [Task 3] ─→ Tail
          ▲                                    │
          └────────── Desbloqueado ──────────┘

Útil para: Justiça, minimizar starvation
Complexidade: O(1) para add/remove
```

### 2. PRIORITY (Ordenado por Prioridade)

```
Inserir em ordem de prioridade (maior primeiro)

Head ─→ [P31] ─→ [P20] ─→ [P10] ─→ Tail
          ▲
          └────── Desbloqueado primeiro

Útil para: Garantir resposta rápida de tarefas críticas
Complexidade: O(n) para add, O(1) para remove_first
```

### 3. TIMEOUT (Ordenado por Tempo de Timeout)

```
Inserir em ordem de tempo de expiração (menor primeiro)

Head ─→ [Expires 1s] ─→ [Expires 2s] ─→ [Expires ∞] ─→ Tail
          ▲
          └────── Próximo a expirar

Útil para: Detectar timeouts eficientemente
Complexidade: O(n) para add, O(1) para processar
```

## API Principal

### Inicialização

```c
int blocked_queue_init(
    blocked_queue_order_t order,  /* FIFO, PRIORITY, ou TIMEOUT */
    uint32_t max_size              /* 0 = ilimitado */
);
```

### Adicionar/Remover

```c
/* Adiciona tarefa à fila */
int blocked_queue_add(
    rtos_task_handle_t task_handle,
    block_type_t block_type,
    uint32_t resource_id,
    uint32_t timeout_ms
);

/* Remove tarefa específica */
blocked_queue_node_t *blocked_queue_remove(
    rtos_task_handle_t task_handle
);

/* Remove primeiro (de acordo com ordem) */
blocked_queue_node_t *blocked_queue_remove_first(void);

/* Remove tarefa com maior prioridade */
blocked_queue_node_t *blocked_queue_remove_highest_priority(void);
```

### Processamento

```c
/* Processa timeouts, retorna número desbloqueado */
uint32_t blocked_queue_process_timeouts(void);

/* Obtém próxima tarefa a sofrer timeout */
blocked_queue_node_t *blocked_queue_get_next_timeout(void);
```

### Consultas

```c
/* Encontra tarefa */
blocked_queue_node_t *blocked_queue_find(
    rtos_task_handle_t task_handle
);

/* Conta tarefas bloqueadas */
uint32_t blocked_queue_get_count(void);

/* Obtém limite de tamanho */
uint32_t blocked_queue_get_max_size(void);

/* Verifica se está cheia */
bool blocked_queue_is_full(void);

/* Verifica se está vazia */
bool blocked_queue_is_empty(void);
```

### Maintenance

```c
/* Limpa toda a fila */
void blocked_queue_clear(void);

/* Muda modo de ordenação (reordena dinamicamente) */
int blocked_queue_set_order(blocked_queue_order_t new_order);

/* Valida integridade */
bool blocked_queue_validate(void);
```

### Debug

```c
/* Imprime conteúdo da fila */
void blocked_queue_print_queue(void);

/* Obtém estatísticas */
void blocked_queue_get_stats(blocked_queue_stats_t *stats);
```

## Exemplo de Uso

### Inicialização com Prioridades

```c
void app_main(void) {
    /* Inicializa kernel */
    rtos_kernel_init();
    
    /* Inicializa fila de bloqueio com ordenação por prioridade */
    blocked_queue_init(BLOCKED_QUEUE_PRIORITY, 0);
    
    /* Cria tarefas */
    rtos_task_create(task_high, "HighPrio", 2048, NULL, 30);
    rtos_task_create(task_low, "LowPrio", 2048, NULL, 10);
    
    rtos_kernel_start();
}
```

### Bloqueio com Fila Automática

```c
rtos_mutex_t mtx = rtos_mutex_create();

void task_high(void *param) {
    /* Tenta adquirir mutex */
    int result = rtos_mutex_lock(mtx, 1000);
    
    /* Se não conseguir, é adicionado à fila de bloqueio
       automaticamente em PRIORITY order */
    
    if (result == RTOS_OK) {
        printf("Task HIGH: lock adquirido\n");
        /* Usa recurso */
        rtos_mutex_unlock(mtx);
    } else {
        printf("Task HIGH: timeout!\n");
    }
}

void task_low(void *param) {
    while (1) {
        int result = rtos_mutex_lock(mtx, RTOS_WAIT_FOREVER);
        
        if (result == RTOS_OK) {
            printf("Task LOW: lock adquirido\n");
            shared_resource++;
            rtos_task_sleep(100);
            rtos_mutex_unlock(mtx);
        }
        rtos_task_sleep(500);
    }
}
```

**Comportamento:**
```
Se Task LOW tiver lock:
  - Task HIGH tenta lock
  - Não consegue (LOW tem)
  - É adicionada à fila em posição PRIORITY
  - Task HIGH fica na frente na fila
  - Quando LOW libera, HIGH é desbloqueada PRIMEIRO
  → Evita priority inversion!
```

### Reordenação Dinâmica

```c
void performance_monitor(void *param) {
    while (1) {
        rtos_task_sleep(5000);  /* A cada 5 segundos */
        
        /* Obtém estatísticas */
        blocked_queue_stats_t stats;
        blocked_queue_get_stats(&stats);
        
        printf("Bloqueadas: %u, Timeouts: %u\n",
               stats.current_blocked,
               stats.timeouts);
        
        /* Se muitos timeouts, muda para TIMEOUT ordering */
        if (stats.timeouts > 10) {
            printf("Muitos timeouts! Mudando para TIMEOUT ordering...\n");
            blocked_queue_set_order(BLOCKED_QUEUE_TIMEOUT);
        }
        
        /* Valida integridade */
        if (!blocked_queue_validate()) {
            printf("ERRO: Fila corrompida!\n");
        }
        
        /* Imprime debug */
        blocked_queue_print_queue();
    }
}
```

## Estrutura de Dados

### Representação em Memória

```
┌─────────────────────────────────────────────────────────┐
│           global_queue (blocked_queue_t)                │
├───────────────────────────────��─────────────────────────┤
│                                                         │
│  head ──┐                                               │
│  tail ──┼────────────────────────────────────────────┐  │
│  count  │ 3                                          │  │
│  order  │ PRIORITY                                   │  │
│         │                                            │  │
└─────────┼────────────────────────────────────────────┼──┘
          │                                            │
          ▼                                            ▼
    ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
    │ Task P31     │◄────▶│ Task P20     │◄────▶│ Task P10     │
    ├──────────────┤      ├──────────────┤      ├──────────────┤
    │ handle: 5    │      │ handle: 7    │      │ handle: 3    │
    │ priority: 31 │      │ priority: 20 │      │ priority: 10 │
    │ timeout: 1s  │      │ timeout: 2s  │      │ timeout: ∞   │
    │ block_time   │      │ block_time   │      │ block_time   │
    │ resource_id  │      │ resource_id  │      │ resource_id  │
    └──────────────┘      └──────────────┘      └──────────────┘
```

## Algoritmos

### Inserção (PRIORITY)

```
insertion_priority(node):
    if fila vazia:
        head = tail = node
        return
    
    current = head
    enquanto current ≠ NULL e current.priority > node.priority:
        current = current.next
    
    se current == NULL:
        /* Insere no final */
        node.prev = tail
        tail.next = node
        tail = node
    senão se current.prev == NULL:
        /* Insere no início */
        node.next = head
        head.prev = node
        head = node
    senão:
        /* Insere no meio */
        node.prev = current.prev
        node.next = current
        current.prev.next = node
        current.prev = node
```

### Reordenação

```
set_order(new_order):
    /* Salva todos os nós */
    nodes[] = extract_all_nodes()
    
    /* Limpa fila */
    clear_queue()
    
    /* Reinserir em nova ordem */
    para cada node em nodes:
        insert_by_order(node, new_order)
```

**Complexidade:** O(n log n) para reordenação completa

### Processamento de Timeouts

```
process_timeouts():
    unblocked = 0
    current = head
    
    enquanto current ≠ NULL:
        elapsed = current_ticks - current.block_time
        
        se current.timeout_ticks == 0:  /* Infinito */
            current = current.next
            continue
        
        se elapsed >= current.timeout_ticks:
            remove_node(current)
            unblocked++
            stats.timeouts++
        
        current = next
    
    retorna unblocked
```

**Complexidade:** O(n) - verifica todos

## Estatísticas

```c
typedef struct {
    uint32_t total_blocked;      /* Total que passaram */
    uint32_t current_blocked;    /* Bloqueados agora */
    uint32_t max_blocked;        /* Pico */
    uint32_t timeouts;           /* Total de timeouts */
    uint32_t avg_wait_time;      /* Tempo médio de espera */
    uint32_t max_wait_time;      /* Tempo máximo de espera */
} blocked_queue_stats_t;
```

### Exemplo de Saída

```
========== Blocked Queue Contents ==========
Total: 3 (max: 0)
Ordem: PRIORITY

# | Task | Type       | Resource | Priority | Timeout
--|------|------------|----------|----------|----------
1 | 5    | SEMAPHORE  | 1        | 31       | 1000 ms
2 | 7    | MUTEX      | 2        | 20       | 2000 ms
3 | 3    | QUEUE_SEND | 3        | 10       | infinite
==========================================
```

## Performance

### Complexidade de Tempo

| Operação | FIFO | PRIORITY | TIMEOUT |
|----------|------|----------|----------|
| add | O(1) | O(n) | O(n) |
| remove | O(1) | O(1) | O(1) |
| remove_first | O(1) | O(1) | O(1) |
| find | O(n) | O(n) | O(n) |
| process_timeouts | O(n) | O(n) | O(n) |
| set_order | O(n log n) | O(n log n) | O(n log n) |

### Overhead de Memória

- **Por nó:** 56 bytes
- **Por fila:** 24 bytes (estrutura)
- **Típico:** 32 tarefas bloqueadas = 56 × 32 = 1.8 KB

## Casos de Uso

### 1. **FIFO** - Justiça
```c
blocked_queue_init(BLOCKED_QUEUE_FIFO, 0);
/* Garante que todas as tarefas sejam desbloqueadas em ordem */
```

### 2. **PRIORITY** - Sistema em Tempo Real Duro
```c
blocked_queue_init(BLOCKED_QUEUE_PRIORITY, 32);
/* Tarefas críticas são desbloqueadas primeiro */
```

### 3. **TIMEOUT** - Aplicações Sensíveis a Latência
```c
blocked_queue_init(BLOCKED_QUEUE_TIMEOUT, 0);
/* Processa timeouts mais eficientemente */
```

## Integração com Sincronização

A fila de bloqueio se integra automaticamente com:

```c
/* Semáforo */
rtos_semaphore_take():
    se não disponível:
        blocked_queue_add(task, BLOCK_SEMAPHORE, sem_id, timeout);
        yield();

/* Mutex */
rtos_mutex_lock():
    se bloqueado:
        blocked_queue_add(task, BLOCK_MUTEX, mtx_id, timeout);
        yield();

/* Fila */
rtos_queue_send():
    se cheia:
        blocked_queue_add(task, BLOCK_QUEUE_SEND, q_id, timeout);
        yield();
```

## Validação

```c
bool blocked_queue_validate(void):
    ✓ Verifica se head/tail são consistentes
    ✓ Verifica se prev/next estão corretos
    ✓ Verifica se count está correto
    ✓ Detecta ciclos
    ✓ Detecta corrupção
```

## Melhores Práticas

✅ **Sempre inicializar antes de usar**
```c
blocked_queue_init(BLOCKED_QUEUE_PRIORITY, 0);
```

✅ **Validar periodicamente**
```c
if (!blocked_queue_validate()) {
    /* Handle erro */
}
```

✅ **Monitorar estatísticas**
```c
blocked_queue_stats_t stats;
blocked_queue_get_stats(&stats);
printf("Bloqueadas: %u\n", stats.current_blocked);
```

✅ **Reordenar se necessário**
```c
if (stats.timeouts > threshold) {
    blocked_queue_set_order(BLOCKED_QUEUE_TIMEOUT);
}
```

❌ **Não acessar nós diretamente**
```c
/* RUIM */
node->priority = 50;  /* Quebra ordenação */

/* BOM */
blocked_queue_remove(task);
blocked_queue_add(task, ...);
```

## Limitações

1. **Tamanho de nó:** 56 bytes (pode ser otimizado)
2. **Sem lock automático:** Cuidado em multi-core
3. **Reordenação:** Pode ser custosa com muitas tarefas
4. **Sem prioridade dinâmica:** Não atualiza se prioridade mudar

## Futuros Melhoramentos

- [ ] Heap binária para inserção O(log n)
- [ ] Red-Black Tree para operações O(log n)
- [ ] Prioridade dinâmica (atualizar se tarefa mudar prioridade)
- [ ] Mecanismo de "priority donation"
- [ ] Lock-free para multi-core
