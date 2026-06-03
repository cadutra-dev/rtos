# Arquitetura do RTOS ESP

## Visão Geral

Este RTOS é um sistema operacional em tempo real leve projetado para microcontroladores ESP. A arquitetura segue um modelo clássico de kernel preemptivo com scheduler baseado em prioridades.

## Componentes Principais

### 1. Kernel (`src/kernel/`)

O núcleo do sistema que gerencia:
- Inicialização e startup
- Contador de ticks do sistema
- Gerenciamento de tarefa atual
- Sincronização com timer de hardware

**Arquivo Principal:** `kernel.c`

```
rtos_kernel_init()  → Inicializa estruturas
rtos_kernel_start() → Inicia scheduler
rtos_kernel_tick()  → Handler chamado por timer
```

### 2. Scheduler (`src/scheduler/`)

Responsável por:
- Criação e remoção de tarefas
- Seleção da próxima tarefa a executar
- Gerenciamento de estados das tarefas
- Implementação do algoritmo de scheduling

**Algoritmo:** Scheduler preemptivo com prioridades
- Seleciona sempre a tarefa pronta com maior prioridade
- Realiza context switch quando necessário

**Estados da Tarefa:**
```
READY      → Pronta para executar
RUNNING    → Executando
BLOCKED    → Aguardando recurso
SLEEPING   → Em sleep (timeout)
SUSPENDED  → Suspensa manualmente
TERMINATED → Terminada
```

### 3. Sincronização (`src/sync/`)

Primitivos de sincronização entre tarefas:

**Semáforos:**
- Contador que pode ser incrementado/decrementado
- Usado para sincronização e controle de acesso

**Mutexes:**
- Exclusão mútua
- Garante que apenas uma tarefa acessa recurso compartilhado
- Previne race conditions

**Filas de Mensagens:**
- Comunicação entre tarefas
- Buffer circular
- Operações enfileirar/desenfileirar

### 4. Gerenciador de Memória (`src/memory/`)

- Alocador simples linear
- Heap fixo de 16KB
- `rtos_malloc()` e `rtos_free()`

### 5. Tratamento de Interrupções (`src/isr/`)

- Handler de ISR (Interrupt Service Routine)
- Chamado por timer de hardware
- Gera ticks do sistema

### 6. Código Específico da Plataforma (`src/platform/esp32/`)

**Context Switching:**
- `context_save()` - Salva contexto da tarefa
- `context_restore()` - Restaura contexto da tarefa

**Timer:**
- `timer_init()` - Configura timer para gerar ticks
- Deve gerar interrupção a cada 1ms

## Fluxo de Execução

### Inicialização

```
app_main()
    ↓
rtos_kernel_init()          // Inicializa kernel
    ├─ scheduler_init()     // Inicializa scheduler
    ├─ memory_init()        // Inicializa heap
    ├─ rtos_task_create()   // Cria tarefas
    └─ task_idle            // Cria tarefa idle
    ↓
rtos_kernel_start()         // Inicia scheduler
    ├─ timer_init()         // Configura timer
    ├─ scheduler_get_next_task()
    └─ context_restore()    // Restaura primeira tarefa
```

### Loop de Execução (Scheduler)

```
Tarefa em execução
    ↓
[Timer gera interrupção a cada 1ms]
    ↓
rtos_kernel_tick()          // Handler de interrupção
    ├─ scheduler_update_tasks()  // Atualiza timeouts
    ├─ scheduler_get_next_task() // Seleciona próxima tarefa
    ├─ context_save()       // Salva contexto atual
    └─ context_restore()    // Restaura nova tarefa
    ↓
Proxima tarefa em execução
```

## Prioridades

- Range: 0-31 (32 níveis)
- 31 = Máxima prioridade
- 0 = Mínima prioridade (tarefa idle)
- Scheduler sempre executa tarefa pronta com maior prioridade

## Timing

- **Tick Rate:** 1000 Hz (1ms)
- **Precisão:** Limitada pela precisão do timer de hardware
- Conversão: `ticks = (ms * 1000) / 1000`

## Limitações

- Máximo 32 tarefas
- Máximo 16 semáforos
- Máximo 16 mutexes
- Máximo 8 filas de mensagens
- Heap fixo de 16KB (sem fragmentação)

## Melhorias Futuras

1. **Context Switching em Assembly:** Para melhor performance
2. **Gerenciador de Memória:** Alocador mais sofisticado
3. **Timeout em Bloqueios:** Implementação completa
4. **Fila de Bloqueio:** Para tarefas aguardando recursos
5. **Herança de Prioridade:** Para evitar priority inversion
6. **Eventos:** Comunicação de múltiplas tarefas
