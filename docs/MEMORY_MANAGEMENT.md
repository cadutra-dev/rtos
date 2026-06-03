# Gerenciador de Memória Avançado

## Visão Geral

O gerenciador de memória do RTOS é um alocador dinâmico de heap sofisticado com suporte a:

- **Múltiplas estratégias de alocação** (First Fit, Best Fit, Worst Fit)
- **Consolidação automática** de blocos livres
- **Compactação manual** para reduzir fragmentação
- **Estatísticas detalhadas** em tempo real
- **Validação de integridade** do heap
- **Análise de fragmentação**

## Características

### 1. Estratégias de Alocação

#### First Fit (Padrão)
```
Busca o primeiro bloco que tenha espaço suficiente
Vantagem: Rápido
Desvantagem: Pode fragmentar mais
Tempo: O(n)
```

#### Best Fit
```
Busca o bloco com melhor ajuste (menor resto)
Vantagem: Reduz fragmentação
Desvantagem: Mais lento
Tempo: O(n)
```

#### Worst Fit
```
Busca o maior bloco disponível
Vantagem: Mantém blocos grandes para futuras alocações
Desvantagem: Muito lento
Tempo: O(n)
```

### 2. Estrutura de Heap

```
┌─────────────────────────────────────┐
│        RTOS Heap (32 KB)            │
├─────────────────────────────────────┤
│                                     │
│  ┌──────────┬──────────┐            │
│  │ Header   │  Dados   │            │
│  └──────────┴──────────┘            │
│   (alocado)                         │
│                                     │
│  ┌────────────────────────────────┐ │
│  │        Bloco Livre             │ │
│  └────────────────────────────────┘ │
│                                     │
└─────────────────────────────────────┘
```

### 3. Estrutura de Bloco

```c
typedef struct memory_block {
    size_t size;                /* Tamanho do bloco em bytes */
    bool allocated;             /* true = alocado, false = livre */
    struct memory_block *next;  /* Próximo bloco */
    struct memory_block *prev;  /* Bloco anterior */
} memory_block_t;  /* 24 bytes em 64-bit */
```

**Layout na Memória:**
```
┌─────────────────────┐
│   memory_block_t    │ ← Header (24 bytes)
│  ┌─────────────────┐│
│  │  Dados Usuário  ││ ← ptr retornado por malloc()
│  │                 ││
│  └─────────────────┘│
└─────────────────────┘
```

## API

### Configuração

```c
int memory_init(void);
void memory_set_strategy(rtos_alloc_strategy_t strategy);
```

### Alocação

```c
void *rtos_malloc(size_t size);
void *rtos_calloc(size_t count, size_t size);
void *rtos_realloc(void *ptr, size_t size);
void rtos_free(void *ptr);
```

### Manutenção

```c
uint32_t memory_compact(void);          /* Consolida blocos livres */
bool memory_validate_heap(void);        /* Valida integridade */
size_t memory_get_block_size(void *ptr);/* Tamanho de bloco alocado */
```

### Monitoramento

```c
void memory_get_stats(rtos_memory_stats_t *stats);
void memory_print_stats(void);
```

## Exemplo de Uso

```c
#include "rtos.h"

void memory_demo(void) {
    /* Inicializa memória */
    memory_init();
    
    /* Aloca blocos */
    int *ptr1 = rtos_malloc(100);
    char *ptr2 = rtos_malloc(50);
    
    /* Usa memória */
    ptr1[0] = 42;
    strcpy(ptr2, "Hello");
    
    /* Obtém estatísticas */
    rtos_memory_stats_t stats;
    memory_get_stats(&stats);
    
    printf("Uso: %u / %u bytes\n", 
           stats.allocated_bytes, 
           stats.total_heap_size);
    printf("Fragmentação: %u%%\n", stats.fragmentation_ratio);
    
    /* Libera memória */
    rtos_free(ptr1);
    rtos_free(ptr2);
    
    /* Compacta */
    uint32_t compacted = memory_compact();
    printf("Blocos consolidados: %u\n", compacted);
    
    /* Imprime relatório */
    memory_print_stats();
}
```

## Algoritmo de Alocação Detalhado

### 1. malloc()

```
1. Valida tamanho > 0 e <= free_bytes
2. Alinha tamanho para 8 bytes
3. Busca bloco livre de acordo com estratégia
4. Se encontrou:
   a. Divide bloco em dois (se houver espaço)
   b. Marca bloco como alocado
   c. Atualiza estatísticas
   d. Retorna ptr após header
5. Senão retorna NULL
```

### 2. free()

```
1. Recupera header do bloco
2. Se não está alocado, retorna (erro)
3. Marca como livre
4. Consolida com bloco anterior se livre
5. Consolida com próximo bloco se livre
6. Atualiza estatísticas
```

### 3. Consolidação

```
Quando bloco é liberado:

┌──────────┐ ┌──────────┐ ┌──────────┐
│ Alocado  │ │  Livre   │ │ Alocado  │
└──────────┘ └──────────┘ └──────────┘
                    ↓
             Consolida com anterior
                    ↓
┌──────────────────────────┐ ┌──────────┐
│       Livre Maior        │ │ Alocado  │
└──────────────────────────┘ └──────────┘
                    ↓
             Se próximo também livre
                    ↓
┌──────────────────────────────────────┐
│       Bloco Livre Máximo             │
└──────────────────────────────────────┘
```

## Estatísticas

### Estrutura de Dados

```c
typedef struct {
    size_t total_heap_size;     /* 32 KB */
    size_t allocated_bytes;     /* Uso atual */
    size_t free_bytes;          /* Disponível */
    size_t fragmentation_ratio; /* % (0-100) */
    uint32_t num_allocations;   /* Ativas */
    uint32_t num_free_blocks;   /* Livres */
    uint32_t peak_allocated;    /* Pico de uso */
    uint32_t total_allocations; /* Histórico */
    uint32_t total_frees;       /* Histórico */
} rtos_memory_stats_t;
```

### Cálculo de Fragmentação

```
fragmentation = 100 - (largest_free_block * 100 / total_free)

Exemplo:
- Total livre: 1000 bytes
- Maior bloco: 900 bytes
- Fragmentação: 100 - (900*100/1000) = 10%

- Total livre: 1000 bytes
- Maior bloco: 100 bytes (muito fragmentado)
- Fragmentação: 100 - (100*100/1000) = 90%
```

## Saída de Exemplo

```
========== RTOS Memory Statistics ==========
Total Heap Size:     32768 bytes (32.0 KB)
Allocated:           8192 bytes (25.0%)
Free:                24576 bytes (75.0%)
Fragmentation:       12%
Active Allocations:  5
Free Blocks:         3
Peak Usage:          12288 bytes
Total Allocations:   42
Total Frees:         37
==========================================
```

## Performance

### Complexidade de Tempo

| Operação | First Fit | Best Fit | Worst Fit |
|----------|-----------|----------|----------|
| malloc() | O(n) | O(n) | O(n) |
| free() | O(1) | O(1) | O(1) |
| compact() | O(n) | O(n) | O(n) |

onde n = número de blocos

### Complexidade de Espaço

- **Overhead por bloco**: 24 bytes (header)
- **Alinhamento**: 8 bytes
- **Total overhead**: ~3% do heap para blocos pequenos

## Otimizações Implementadas

1. **Split Block**: Divide blocos maiores para reduzir espaço desperdiçado
2. **Coalescing**: Consolida blocos livres adjacentes automaticamente
3. **Alignment**: Alinha alocações para melhor performance
4. **Statistics**: Rastreia pico de uso para planejamento
5. **Validation**: Valida integridade do heap para detectar corrupção

## Casos de Uso

### Best Fit - Para Aplicações Sensíveis a Fragmentação
```c
memory_set_strategy(RTOS_MEM_BEST_FIT);
/* Bom para muitas alocações de tamanhos diversos */
```

### Worst Fit - Para Alocações Balanceadas
```c
memory_set_strategy(RTOS_MEM_WORST_FIT);
/* Mantém blocos grandes para futuras necessidades */
```

### First Fit - Para Performance Máxima (Padrão)
```c
/* Rápido e simples, bom para maioria dos casos */
```

## Limitações e Considerações

1. **Heap fixo**: Não cresce dinamicamente
2. **Sem thread-safety**: Deve ser usado com sincronização
3. **Sem garbage collection**: Vazamentos se não liberar
4. **Realocação**: Pode ser cara em heaps fragmentados
5. **Debug**: Use `memory_validate_heap()` regularmente

## Monitoramento Recomendado

```c
/* A cada 1 segundo */
if (rtos_kernel_get_ticks() % 1000 == 0) {
    memory_print_stats();
    
    if (!memory_validate_heap()) {
        printf("ERRO: Heap corrompido!\n");
    }
    
    /* Compacta se fragmentação > 50% */
    rtos_memory_stats_t stats;
    memory_get_stats(&stats);
    if (stats.fragmentation_ratio > 50) {
        uint32_t c = memory_compact();
        printf("Heap compactado: %u blocos\n", c);
    }
}
```

## Futuros Melhoramentos

- [ ] Memory pools pré-alocados para tamanhos fixos
- [ ] Buddy allocator para melhor fragmentação
- [ ] Slab allocator para objetos do kernel
- [ ] DMA-safe memory regions
- [ ] RTOS-aware garbage collection
- [ ] Profiling integrado com rastreamento de quem aloca
