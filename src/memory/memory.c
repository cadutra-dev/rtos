#include "../include/memory.h"
#include "../../include/rtos.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ============================================
   Variáveis Globais
   ============================================ */

/* Heap estático */
static uint8_t heap[RTOS_HEAP_SIZE] __attribute__((aligned(RTOS_HEAP_ALIGN)));

/* Cabeçalho do heap (primeiro bloco) */
static memory_block_t *heap_head = NULL;

/* Estratégia de alocação atual */
static rtos_alloc_strategy_t alloc_strategy = RTOS_MEM_FIRST_FIT;

/* Estatísticas */
static rtos_memory_stats_t memory_stats = {0};

/* ============================================
   Funções Auxiliares Internas
   ============================================ */

/**
 * @brief Alinha tamanho para requisito de alinhamento
 */
static size_t align_size(size_t size) {
    return (size + RTOS_HEAP_ALIGN - 1) & ~(RTOS_HEAP_ALIGN - 1);
}

/**
 * @brief Encontra um bloco livre usando First Fit
 */
static memory_block_t *find_first_fit(size_t size) {
    memory_block_t *block = heap_head;
    
    while (block != NULL) {
        if (!block->allocated && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    return NULL;
}

/**
 * @brief Encontra um bloco livre usando Best Fit
 */
static memory_block_t *find_best_fit(size_t size) {
    memory_block_t *block = heap_head;
    memory_block_t *best = NULL;
    size_t best_diff = SIZE_MAX;
    
    while (block != NULL) {
        if (!block->allocated && block->size >= size) {
            size_t diff = block->size - size;
            if (diff < best_diff) {
                best = block;
                best_diff = diff;
            }
        }
        block = block->next;
    }
    return best;
}

/**
 * @brief Encontra um bloco livre usando Worst Fit
 */
static memory_block_t *find_worst_fit(size_t size) {
    memory_block_t *block = heap_head;
    memory_block_t *worst = NULL;
    size_t worst_size = 0;
    
    while (block != NULL) {
        if (!block->allocated && block->size >= size && block->size > worst_size) {
            worst = block;
            worst_size = block->size;
        }
        block = block->next;
    }
    return worst;
}

/**
 * @brief Encontra um bloco livre de acordo com a estratégia
 */
static memory_block_t *find_free_block(size_t size) {
    switch (alloc_strategy) {
        case RTOS_MEM_FIRST_FIT:
            return find_first_fit(size);
        case RTOS_MEM_BEST_FIT:
            return find_best_fit(size);
        case RTOS_MEM_WORST_FIT:
            return find_worst_fit(size);
        default:
            return find_first_fit(size);
    }
}

/**
 * @brief Divide um bloco em dois (para reduzir fragmentação)
 */
static memory_block_t *split_block(memory_block_t *block, size_t size) {
    if (block->size <= size + sizeof(memory_block_t)) {
        return block;
    }
    
    /* Cria novo bloco para a parte restante */
    memory_block_t *new_block = (memory_block_t *)(
        (uintptr_t)block + sizeof(memory_block_t) + size
    );
    
    new_block->size = block->size - size - sizeof(memory_block_t);
    new_block->allocated = false;
    new_block->next = block->next;
    new_block->prev = block;
    
    if (block->next != NULL) {
        block->next->prev = new_block;
    }
    
    block->next = new_block;
    block->size = size;
    
    return block;
}

/**
 * @brief Calcula taxa de fragmentação
 */
static uint32_t calculate_fragmentation(void) {
    if (memory_stats.free_bytes == 0) {
        return 0;
    }
    
    memory_block_t *block = heap_head;
    uint32_t largest_free = 0;
    uint32_t num_free_blocks = 0;
    
    while (block != NULL) {
        if (!block->allocated) {
            if (block->size > largest_free) {
                largest_free = block->size;
            }
            num_free_blocks++;
        }
        block = block->next;
    }
    
    memory_stats.num_free_blocks = num_free_blocks;
    
    if (largest_free == 0) {
        return 100;
    }
    
    return 100 - ((largest_free * 100) / memory_stats.free_bytes);
}

/* ============================================
   API Pública
   ============================================ */

int memory_init(void) {
    /* Zera o heap */
    memset(heap, 0, RTOS_HEAP_SIZE);
    
    /* Inicializa bloco raiz */
    heap_head = (memory_block_t *)heap;
    heap_head->size = RTOS_HEAP_SIZE - sizeof(memory_block_t);
    heap_head->allocated = false;
    heap_head->next = NULL;
    heap_head->prev = NULL;
    
    /* Inicializa estatísticas */
    memory_stats.total_heap_size = RTOS_HEAP_SIZE;
    memory_stats.allocated_bytes = 0;
    memory_stats.free_bytes = heap_head->size;
    memory_stats.fragmentation_ratio = 0;
    memory_stats.num_allocations = 0;
    memory_stats.num_free_blocks = 1;
    memory_stats.peak_allocated = 0;
    memory_stats.total_allocations = 0;
    memory_stats.total_frees = 0;
    
    printf("[RTOS Memory] Initialized: %u KB, Strategy: FIRST_FIT\n", 
           RTOS_HEAP_SIZE / 1024);
    
    return RTOS_OK;
}

void memory_set_strategy(rtos_alloc_strategy_t strategy) {
    alloc_strategy = strategy;
    const char *names[] = {"FIRST_FIT", "BEST_FIT", "WORST_FIT"};
    printf("[RTOS Memory] Strategy changed to: %s\n", names[strategy]);
}

void *rtos_malloc(size_t size) {
    if (size == 0 || size > memory_stats.free_bytes) {
        return NULL;
    }
    
    size = align_size(size);
    
    /* Encontra bloco livre */
    memory_block_t *block = find_free_block(size);
    if (block == NULL) {
        return NULL;
    }
    
    /* Divide bloco se necessário */
    split_block(block, size);
    
    /* Marca como alocado */
    block->allocated = true;
    
    /* Atualiza estatísticas */
    memory_stats.allocated_bytes += size;
    memory_stats.free_bytes -= size + sizeof(memory_block_t);
    memory_stats.num_allocations++;
    memory_stats.total_allocations++;
    
    if (memory_stats.allocated_bytes > memory_stats.peak_allocated) {
        memory_stats.peak_allocated = memory_stats.allocated_bytes;
    }
    
    /* Retorna ponteiro para dados (após header do bloco) */
    return (void *)((uintptr_t)block + sizeof(memory_block_t));
}

void rtos_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    
    /* Recupera header do bloco */
    memory_block_t *block = (memory_block_t *)((uintptr_t)ptr - sizeof(memory_block_t));
    
    if (!block->allocated) {
        printf("[RTOS Memory] WARNING: Tentativa de liberar memória não alocada\n");
        return;
    }
    
    block->allocated = false;
    
    /* Atualiza estatísticas */
    memory_stats.allocated_bytes -= block->size;
    memory_stats.free_bytes += block->size + sizeof(memory_block_t);
    memory_stats.num_allocations--;
    memory_stats.total_frees++;
    
    /* Consolida blocos adjacentes livres */
    if (block->next != NULL && !block->next->allocated) {
        block->size += sizeof(memory_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next != NULL) {
            block->next->prev = block;
        }
    }
    
    if (block->prev != NULL && !block->prev->allocated) {
        block->prev->size += sizeof(memory_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next != NULL) {
            block->next->prev = block->prev;
        }
    }
}

void *rtos_calloc(size_t count, size_t size) {
    size_t total_size = count * size;
    if (total_size == 0 || count > SIZE_MAX / size) {
        return NULL;
    }
    
    void *ptr = rtos_malloc(total_size);
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

void *rtos_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return rtos_malloc(size);
    }
    
    if (size == 0) {
        rtos_free(ptr);
        return NULL;
    }
    
    /* Recupera header do bloco original */
    memory_block_t *old_block = (memory_block_t *)((uintptr_t)ptr - sizeof(memory_block_t));
    
    /* Se novo tamanho cabe no bloco atual, retorna o mesmo */
    if (old_block->size >= align_size(size)) {
        return ptr;
    }
    
    /* Aloca novo bloco */
    void *new_ptr = rtos_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    /* Copia dados */
    memcpy(new_ptr, ptr, old_block->size);
    
    /* Libera bloco antigo */
    rtos_free(ptr);
    
    return new_ptr;
}

void memory_get_stats(rtos_memory_stats_t *stats) {
    if (stats == NULL) {
        return;
    }
    
    memory_stats.fragmentation_ratio = calculate_fragmentation();
    memcpy(stats, &memory_stats, sizeof(rtos_memory_stats_t));
}

uint32_t memory_compact(void) {
    uint32_t compacted = 0;
    memory_block_t *block = heap_head;
    
    while (block != NULL && block->next != NULL) {
        if (!block->allocated && !block->next->allocated) {
            /* Consolida blocos livres adjacentes */
            block->size += sizeof(memory_block_t) + block->next->size;
            block->next = block->next->next;
            if (block->next != NULL) {
                block->next->prev = block;
            }
            compacted++;
        } else {
            block = block->next;
        }
    }
    
    return compacted;
}

void memory_print_stats(void) {
    memory_get_stats(&memory_stats);
    
    printf("\n========== RTOS Memory Statistics ==========");
    printf("\nTotal Heap Size:     %u bytes (%.1f KB)", 
           memory_stats.total_heap_size,
           memory_stats.total_heap_size / 1024.0);
    printf("\nAllocated:           %u bytes (%.1f%%)", 
           memory_stats.allocated_bytes,
           (memory_stats.allocated_bytes * 100.0) / memory_stats.total_heap_size);
    printf("\nFree:                %u bytes (%.1f%%)", 
           memory_stats.free_bytes,
           (memory_stats.free_bytes * 100.0) / memory_stats.total_heap_size);
    printf("\nFragmentation:       %u%%", memory_stats.fragmentation_ratio);
    printf("\nActive Allocations:  %u", memory_stats.num_allocations);
    printf("\nFree Blocks:         %u", memory_stats.num_free_blocks);
    printf("\nPeak Usage:          %u bytes", memory_stats.peak_allocated);
    printf("\nTotal Allocations:   %u", memory_stats.total_allocations);
    printf("\nTotal Frees:         %u", memory_stats.total_frees);
    printf("\n=========================================\n");
}

bool memory_validate_heap(void) {
    memory_block_t *block = heap_head;
    size_t total_size = 0;
    
    while (block != NULL) {
        /* Verifica se tamanho é válido */
        if (block->size == 0 || block->size > RTOS_HEAP_SIZE) {
            printf("[RTOS Memory] ERRO: Tamanho de bloco inválido\n");
            return false;
        }
        
        total_size += block->size + sizeof(memory_block_t);
        
        /* Verifica encadeamento */
        if (block->next != NULL && block->next->prev != block) {
            printf("[RTOS Memory] ERRO: Encadeamento de blocos corrompido\n");
            return false;
        }
        
        block = block->next;
    }
    
    /* Verifica se total não excede heap */
    if (total_size > RTOS_HEAP_SIZE) {
        printf("[RTOS Memory] ERRO: Tamanho total excede heap\n");
        return false;
    }
    
    printf("[RTOS Memory] Heap validation: OK\n");
    return true;
}

size_t memory_get_block_size(void *ptr) {
    if (ptr == NULL) {
        return 0;
    }
    
    memory_block_t *block = (memory_block_t *)((uintptr_t)ptr - sizeof(memory_block_t));
    
    if (!block->allocated) {
        return 0;
    }
    
    return block->size;
}
