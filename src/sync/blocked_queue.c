#include "../include/blocked_queue.h"
#include "../../include/rtos.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================
   Variáveis Globais
   ============================================ */

static blocked_queue_t global_queue = {0};
static blocked_queue_order_t current_order = BLOCKED_QUEUE_FIFO;
static blocked_queue_stats_t queue_stats = {0};
static bool queue_initialized = false;

/* ============================================
   Funções Auxiliares Internas
   ============================================ */

/**
 * @brief Converte ms em ticks
 */
static uint32_t ms_to_ticks(uint32_t ms) {
    if (ms == 0) return 0;
    return (ms * RTOS_TICK_RATE_HZ) / 1000;
}

/**
 * @brief Converte ticks em ms
 */
static uint32_t ticks_to_ms(uint32_t ticks) {
    if (ticks == 0) return 0;
    return (ticks * 1000) / RTOS_TICK_RATE_HZ;
}

/**
 * @brief Cria novo nó
 */
static blocked_queue_node_t *create_node(
    rtos_task_handle_t task_handle,
    block_type_t block_type,
    uint32_t resource_id,
    uint8_t priority,
    uint32_t timeout_ms
) {
    blocked_queue_node_t *node = 
        (blocked_queue_node_t *)rtos_malloc(sizeof(blocked_queue_node_t));
    
    if (node == NULL) {
        printf("[RTOS BlockedQueue] ERRO: Sem memória para novo nó\n");
        return NULL;
    }
    
    node->task_handle = task_handle;
    node->resource_id = resource_id;
    node->block_type = block_type;
    node->priority = priority;
    node->timeout_ticks = ms_to_ticks(timeout_ms);
    node->block_time = rtos_kernel_get_ticks();
    node->wait_time = 0;
    node->prev = NULL;
    node->next = NULL;
    
    return node;
}

/**
 * @brief Insere nó em ordem FIFO
 */
static void insert_fifo(blocked_queue_node_t *node) {
    if (global_queue.head == NULL) {
        global_queue.head = node;
        global_queue.tail = node;
    } else {
        node->prev = global_queue.tail;
        global_queue.tail->next = node;
        global_queue.tail = node;
    }
}

/**
 * @brief Insere nó em ordem de prioridade (maior primeiro)
 */
static void insert_priority(blocked_queue_node_t *node) {
    if (global_queue.head == NULL) {
        global_queue.head = node;
        global_queue.tail = node;
        return;
    }
    
    /* Procura posição correta */
    blocked_queue_node_t *current = global_queue.head;
    
    while (current != NULL && current->priority > node->priority) {
        current = current->next;
    }
    
    if (current == NULL) {
        /* Insere no final */
        node->prev = global_queue.tail;
        global_queue.tail->next = node;
        global_queue.tail = node;
    } else if (current->prev == NULL) {
        /* Insere no início */
        node->next = global_queue.head;
        global_queue.head->prev = node;
        global_queue.head = node;
    } else {
        /* Insere no meio */
        node->prev = current->prev;
        node->next = current;
        current->prev->next = node;
        current->prev = node;
    }
}

/**
 * @brief Insere nó em ordem de timeout (menor tempo restante primeiro)
 */
static void insert_timeout(blocked_queue_node_t *node) {
    if (global_queue.head == NULL) {
        global_queue.head = node;
        global_queue.tail = node;
        return;
    }
    
    uint32_t current_ticks = rtos_kernel_get_ticks();
    uint32_t node_timeout = node->block_time + node->timeout_ticks;
    
    /* Procura posição correta */
    blocked_queue_node_t *current = global_queue.head;
    
    while (current != NULL) {
        uint32_t current_timeout = current->block_time + current->timeout_ticks;
        if (node_timeout <= current_timeout) {
            break;
        }
        current = current->next;
    }
    
    if (current == NULL) {
        /* Insere no final */
        node->prev = global_queue.tail;
        global_queue.tail->next = node;
        global_queue.tail = node;
    } else if (current->prev == NULL) {
        /* Insere no início */
        node->next = global_queue.head;
        global_queue.head->prev = node;
        global_queue.head = node;
    } else {
        /* Insere no meio */
        node->prev = current->prev;
        node->next = current;
        current->prev->next = node;
        current->prev = node;
    }
}

/**
 * @brief Remove nó da fila (sem liberar)
 */
static void remove_node(blocked_queue_node_t *node) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        global_queue.head = node->next;
    }
    
    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        global_queue.tail = node->prev;
    }
    
    global_queue.count--;
}

/**
 * @brief Reordena toda a fila
 */
static void reorder_queue(blocked_queue_order_t new_order) {
    if (global_queue.head == NULL) {
        return;
    }
    
    /* Salva todos os nós */
    blocked_queue_node_t **nodes = 
        (blocked_queue_node_t **)rtos_malloc(
            sizeof(blocked_queue_node_t *) * global_queue.count
        );
    
    if (nodes == NULL) {
        printf("[RTOS BlockedQueue] ERRO: Sem memória para reordenação\n");
        return;
    }
    
    /* Extrai todos os nós */
    uint32_t count = 0;
    blocked_queue_node_t *current = global_queue.head;
    while (current != NULL) {
        nodes[count++] = current;
        current = current->next;
    }
    
    /* Limpa fila */
    global_queue.head = NULL;
    global_queue.tail = NULL;
    
    /* Reinserir em nova ordem */
    for (uint32_t i = 0; i < count; i++) {
        nodes[i]->prev = NULL;
        nodes[i]->next = NULL;
        
        switch (new_order) {
            case BLOCKED_QUEUE_FIFO:
                insert_fifo(nodes[i]);
                break;
            case BLOCKED_QUEUE_PRIORITY:
                insert_priority(nodes[i]);
                break;
            case BLOCKED_QUEUE_TIMEOUT:
                insert_timeout(nodes[i]);
                break;
        }
    }
    
    rtos_free(nodes);
}

/* ============================================
   API Pública
   ============================================ */

int blocked_queue_init(blocked_queue_order_t order, uint32_t max_size) {
    if (queue_initialized) {
        printf("[RTOS BlockedQueue] Já inicializado\n");
        return RTOS_ERROR;
    }
    
    memset(&global_queue, 0, sizeof(blocked_queue_t));
    memset(&queue_stats, 0, sizeof(blocked_queue_stats_t));
    
    global_queue.order = order;
    global_queue.max_size = max_size;
    current_order = order;
    queue_initialized = true;
    
    const char *order_names[] = {"FIFO", "PRIORITY", "TIMEOUT"};
    printf("[RTOS BlockedQueue] Inicializado (ordem: %s, max: %u)\n",
           order_names[order], max_size);
    
    return RTOS_OK;
}

int blocked_queue_add(
    rtos_task_handle_t task_handle,
    block_type_t block_type,
    uint32_t resource_id,
    uint32_t timeout_ms
) {
    if (!queue_initialized) {
        printf("[RTOS BlockedQueue] Fila não inicializada\n");
        return RTOS_ERROR;
    }
    
    /* Verifica limite de tamanho */
    if (global_queue.max_size > 0 && global_queue.count >= global_queue.max_size) {
        printf("[RTOS BlockedQueue] ERRO: Fila cheia\n");
        return RTOS_ERROR;
    }
    
    /* Obtém prioridade da tarefa */
    uint8_t priority = rtos_task_get_priority(task_handle);
    
    /* Cria novo nó */
    blocked_queue_node_t *node = create_node(
        task_handle, block_type, resource_id, priority, timeout_ms
    );
    
    if (node == NULL) {
        return RTOS_ERROR;
    }
    
    /* Insere de acordo com modo */
    switch (global_queue.order) {
        case BLOCKED_QUEUE_FIFO:
            insert_fifo(node);
            break;
        case BLOCKED_QUEUE_PRIORITY:
            insert_priority(node);
            break;
        case BLOCKED_QUEUE_TIMEOUT:
            insert_timeout(node);
            break;
    }
    
    global_queue.count++;
    
    /* Atualiza estatísticas */
    queue_stats.current_blocked = global_queue.count;
    queue_stats.total_blocked++;
    if (global_queue.count > queue_stats.max_blocked) {
        queue_stats.max_blocked = global_queue.count;
    }
    
    printf("[RTOS BlockedQueue] Tarefa %u bloqueada (total: %u)\n",
           task_handle, global_queue.count);
    
    return RTOS_OK;
}

blocked_queue_node_t *blocked_queue_remove(rtos_task_handle_t task_handle) {
    if (global_queue.head == NULL) {
        return NULL;
    }
    
    blocked_queue_node_t *current = global_queue.head;
    
    while (current != NULL) {
        if (current->task_handle == task_handle) {
            remove_node(current);
            queue_stats.current_blocked = global_queue.count;
            printf("[RTOS BlockedQueue] Tarefa %u removida (total: %u)\n",
                   task_handle, global_queue.count);
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

blocked_queue_node_t *blocked_queue_remove_first(void) {
    if (global_queue.head == NULL) {
        return NULL;
    }
    
    blocked_queue_node_t *node = global_queue.head;
    remove_node(node);
    queue_stats.current_blocked = global_queue.count;
    
    return node;
}

blocked_queue_node_t *blocked_queue_remove_highest_priority(void) {
    if (global_queue.head == NULL) {
        return NULL;
    }
    
    /* Se está em PRIORITY order, o primeiro é de maior prioridade */
    if (global_queue.order == BLOCKED_QUEUE_PRIORITY) {
        return blocked_queue_remove_first();
    }
    
    /* Senão, busca o de maior prioridade */
    blocked_queue_node_t *current = global_queue.head;
    blocked_queue_node_t *highest = current;
    
    while (current != NULL) {
        if (current->priority > highest->priority) {
            highest = current;
        }
        current = current->next;
    }
    
    remove_node(highest);
    queue_stats.current_blocked = global_queue.count;
    
    return highest;
}

uint32_t blocked_queue_process_timeouts(void) {
    if (global_queue.head == NULL) {
        return 0;
    }
    
    uint32_t current_ticks = rtos_kernel_get_ticks();
    uint32_t unblocked = 0;
    
    blocked_queue_node_t *current = global_queue.head;
    
    while (current != NULL) {
        blocked_queue_node_t *next = current->next;  /* Salva próximo */
        
        /* Se timeout é infinito, pula */
        if (current->timeout_ticks == 0) {
            current = next;
            continue;
        }
        
        /* Calcula tempo decorrido */
        uint32_t elapsed = current_ticks - current->block_time;
        
        /* Se timeout expirou */
        if (elapsed >= current->timeout_ticks) {
            printf("[RTOS BlockedQueue] TIMEOUT: Tarefa %u (elapsed: %u ticks)\n",
                   current->task_handle, elapsed);
            
            /* Atualiza tempo de espera */
            queue_stats.timeouts++;
            uint32_t wait = ticks_to_ms(elapsed);
            queue_stats.avg_wait_time = (queue_stats.avg_wait_time + wait) / 2;
            if (wait > queue_stats.max_wait_time) {
                queue_stats.max_wait_time = wait;
            }
            
            remove_node(current);
            unblocked++;
        }
        
        current = next;
    }
    
    queue_stats.current_blocked = global_queue.count;
    return unblocked;
}

blocked_queue_node_t *blocked_queue_find(rtos_task_handle_t task_handle) {
    blocked_queue_node_t *current = global_queue.head;
    
    while (current != NULL) {
        if (current->task_handle == task_handle) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

blocked_queue_node_t *blocked_queue_get_next_timeout(void) {
    if (global_queue.head == NULL) {
        return NULL;
    }
    
    uint32_t current_ticks = rtos_kernel_get_ticks();
    blocked_queue_node_t *current = global_queue.head;
    blocked_queue_node_t *next_timeout = NULL;
    uint32_t min_time = UINT32_MAX;
    
    while (current != NULL) {
        if (current->timeout_ticks > 0) {
            uint32_t timeout_at = current->block_time + current->timeout_ticks;
            if (timeout_at < min_time) {
                min_time = timeout_at;
                next_timeout = current;
            }
        }
        current = current->next;
    }
    
    return next_timeout;
}

uint32_t blocked_queue_get_count(void) {
    return global_queue.count;
}

uint32_t blocked_queue_get_max_size(void) {
    return global_queue.max_size;
}

bool blocked_queue_is_full(void) {
    if (global_queue.max_size == 0) {
        return false;  /* Ilimitado */
    }
    return global_queue.count >= global_queue.max_size;
}

bool blocked_queue_is_empty(void) {
    return global_queue.count == 0;
}

void blocked_queue_clear(void) {
    blocked_queue_node_t *current = global_queue.head;
    
    while (current != NULL) {
        blocked_queue_node_t *next = current->next;
        rtos_free(current);
        current = next;
    }
    
    global_queue.head = NULL;
    global_queue.tail = NULL;
    global_queue.count = 0;
    queue_stats.current_blocked = 0;
    
    printf("[RTOS BlockedQueue] Fila limpa\n");
}

void blocked_queue_get_stats(blocked_queue_stats_t *stats) {
    if (stats == NULL) {
        return;
    }
    memcpy(stats, &queue_stats, sizeof(blocked_queue_stats_t));
}

void blocked_queue_print_queue(void) {
    printf("\n========== Blocked Queue Contents ==========");
    printf("\nTotal: %u (max: %u)\n", global_queue.count, global_queue.max_size);
    
    if (global_queue.head == NULL) {
        printf("(vazio)\n");
        printf("==========================================\n");
        return;
    }
    
    const char *block_types[] = {"NONE", "SEMAPHORE", "MUTEX", "QUEUE_SEND", "QUEUE_RCV"};
    const char *order_names[] = {"FIFO", "PRIORITY", "TIMEOUT"};
    
    printf("Ordem: %s\n", order_names[global_queue.order]);
    printf("\n# | Task | Type       | Resource | Priority | Timeout\n");
    printf("--|------|------------|----------|----------|----------\n");
    
    blocked_queue_node_t *current = global_queue.head;
    uint32_t index = 1;
    
    while (current != NULL) {
        uint32_t current_ticks = rtos_kernel_get_ticks();
        uint32_t elapsed = current_ticks - current->block_time;
        
        printf("%u | %4u | %-10s | %8u | %8u | ",
               index,
               current->task_handle,
               block_types[current->block_type],
               current->resource_id,
               current->priority);
        
        if (current->timeout_ticks == 0) {
            printf("infinite\n");
        } else {
            uint32_t remaining = current->timeout_ticks - elapsed;
            printf("%u ms\n", ticks_to_ms(remaining));
        }
        
        current = current->next;
        index++;
    }
    printf("==========================================\n");
}

bool blocked_queue_validate(void) {
    /* Verifica se head e tail são consistentes */
    if ((global_queue.head == NULL) != (global_queue.count == 0)) {
        printf("[RTOS BlockedQueue] ERRO: head/tail inconsistente\n");
        return false;
    }
    
    if (global_queue.count == 0 && global_queue.tail != NULL) {
        printf("[RTOS BlockedQueue] ERRO: tail deve ser NULL quando vazio\n");
        return false;
    }
    
    /* Verifica encadeamento */
    uint32_t count = 0;
    blocked_queue_node_t *current = global_queue.head;
    blocked_queue_node_t *prev = NULL;
    
    while (current != NULL) {
        if (current->prev != prev) {
            printf("[RTOS BlockedQueue] ERRO: ponteiro prev inconsistente\n");
            return false;
        }
        
        count++;
        prev = current;
        current = current->next;
    }
    
    if (prev != global_queue.tail) {
        printf("[RTOS BlockedQueue] ERRO: tail inconsistente\n");
        return false;
    }
    
    if (count != global_queue.count) {
        printf("[RTOS BlockedQueue] ERRO: contagem inconsistente\n");
        return false;
    }
    
    printf("[RTOS BlockedQueue] Validação: OK (%u nós)\n", count);
    return true;
}

int blocked_queue_set_order(blocked_queue_order_t new_order) {
    if (new_order == global_queue.order) {
        return RTOS_OK;
    }
    
    printf("[RTOS BlockedQueue] Reordenando fila...\n");
    reorder_queue(new_order);
    global_queue.order = new_order;
    current_order = new_order;
    
    const char *order_names[] = {"FIFO", "PRIORITY", "TIMEOUT"};
    printf("[RTOS BlockedQueue] Nova ordem: %s\n", order_names[new_order]);
    
    return RTOS_OK;
}
