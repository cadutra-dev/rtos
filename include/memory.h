#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================
   Configuração de Memória
   ============================================ */

#define RTOS_HEAP_SIZE          (32 * 1024)     /* 32 KB */
#define RTOS_MIN_BLOCK_SIZE     16              /* Mínimo de bytes por bloco */
#define RTOS_HEAP_ALIGN         8               /* Alinhamento de heap */

/* ============================================
   Tipos de Alocação
   ============================================ */

typedef enum {
    RTOS_MEM_FIRST_FIT,         /* Primeiro espaço que cabe */
    RTOS_MEM_BEST_FIT,          /* Melhor ajuste (menor resto) */
    RTOS_MEM_WORST_FIT          /* Pior ajuste (maior espaço) */
} rtos_alloc_strategy_t;

/* ============================================
   Estrutura de Bloco de Memória
   ============================================ */

typedef struct memory_block {
    size_t size;                /* Tamanho do bloco */
    bool allocated;             /* Se está alocado */
    struct memory_block *next;  /* Próximo bloco */
    struct memory_block *prev;  /* Bloco anterior */
} memory_block_t;

/* ============================================
   Estrutura de Estatísticas
   ============================================ */

typedef struct {
    size_t total_heap_size;     /* Tamanho total do heap */
    size_t allocated_bytes;     /* Bytes alocados */
    size_t free_bytes;          /* Bytes livres */
    size_t fragmentation_ratio; /* Taxa de fragmentação (0-100%) */
    uint32_t num_allocations;   /* Número de alocações ativas */
    uint32_t num_free_blocks;   /* Número de blocos livres */
    uint32_t peak_allocated;    /* Pico de uso */
    uint32_t total_allocations; /* Total de alocações (histórico) */
    uint32_t total_frees;       /* Total de liberações (histórico) */
} rtos_memory_stats_t;

/* ============================================
   API de Memória
   ============================================ */

/**
 * @brief Inicializa o gerenciador de memória
 * @return RTOS_OK em caso de sucesso
 */
int memory_init(void);

/**
 * @brief Define a estratégia de alocação
 * @param strategy Estratégia a usar
 */
void memory_set_strategy(rtos_alloc_strategy_t strategy);

/**
 * @brief Aloca memória
 * @param size Tamanho em bytes
 * @return Ponteiro para memória ou NULL se falhar
 */
void *rtos_malloc(size_t size);

/**
 * @brief Libera memória previamente alocada
 * @param ptr Ponteiro para memória
 */
void rtos_free(void *ptr);

/**
 * @brief Aloca e zera memória (calloc)
 * @param count Número de elementos
 * @param size Tamanho de cada elemento
 * @return Ponteiro para memória ou NULL se falhar
 */
void *rtos_calloc(size_t count, size_t size);

/**
 * @brief Realoca memória
 * @param ptr Ponteiro anterior (ou NULL)
 * @param size Novo tamanho
 * @return Novo ponteiro ou NULL se falhar
 */
void *rtos_realloc(void *ptr, size_t size);

/**
 * @brief Obtém estatísticas de memória
 * @param stats Ponteiro para estrutura a preencher
 */
void memory_get_stats(rtos_memory_stats_t *stats);

/**
 * @brief Compacta memória (consolidar blocos livres)
 * @return Número de blocos consolidados
 */
uint32_t memory_compact(void);

/**
 * @brief Imprime relatório de memória
 */
void memory_print_stats(void);

/**
 * @brief Valida integridade do heap
 * @return true se heap está íntegro, false caso contrário
 */
bool memory_validate_heap(void);

/**
 * @brief Obtém tamanho de um bloco alocado
 * @param ptr Ponteiro para memória alocada
 * @return Tamanho em bytes (0 se inválido)
 */
size_t memory_get_block_size(void *ptr);

#endif /* MEMORY_H */
