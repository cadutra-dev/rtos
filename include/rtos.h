#ifndef RTOS_H
#define RTOS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
   Tipos e Definições
   ============================================ */

#define RTOS_MAX_TASKS          32
#define RTOS_MAX_SEMAPHORES     16
#define RTOS_MAX_MUTEXES        16
#define RTOS_MAX_QUEUES         8
#define RTOS_TICK_RATE_HZ       1000    // 1ms tick

typedef void (*rtos_task_function_t)(void *);

/* Estados da tarefa */
typedef enum {
    RTOS_TASK_READY,
    RTOS_TASK_RUNNING,
    RTOS_TASK_BLOCKED,
    RTOS_TASK_SLEEPING,
    RTOS_TASK_SUSPENDED,
    RTOS_TASK_TERMINATED
} rtos_task_state_t;

/* Handle para tarefa */
typedef uint32_t rtos_task_handle_t;

/* Handle para semáforo */
typedef uint32_t rtos_semaphore_t;

/* Handle para mutex */
typedef uint32_t rtos_mutex_t;

/* Handle para fila */
typedef uint32_t rtos_queue_t;

/* Códigos de retorno */
typedef enum {
    RTOS_OK = 0,
    RTOS_ERROR = -1,
    RTOS_TIMEOUT = -2,
    RTOS_NO_MEMORY = -3,
    RTOS_INVALID_PARAM = -4
} rtos_error_t;

/* ============================================
   API do Kernel
   ============================================ */

/**
 * @brief Inicializa o kernel RTOS
 * @return RTOS_OK em caso de sucesso
 */
int rtos_kernel_init(void);

/**
 * @brief Inicia o scheduler e executa as tarefas
 * @return Nunca retorna (kernel roda indefinidamente)
 */
int rtos_kernel_start(void);

/**
 * @brief Handler de tick do kernel (chamado por timer)
 * Deve ser chamado a cada tick (normalmente 1ms)
 */
void rtos_kernel_tick(void);

/**
 * @brief Obtém o tempo decorrido em ms desde boot
 * @return Tempo em milissegundos
 */
uint32_t rtos_kernel_get_ticks(void);

/* ============================================
   API de Tarefas
   ============================================ */

/**
 * @brief Cria uma nova tarefa
 * @param func Função que será executada pela tarefa
 * @param name Nome da tarefa (até 32 caracteres)
 * @param stack_size Tamanho da stack em bytes
 * @param param Parâmetro passado para a função
 * @param priority Prioridade (0-31, maior = mais alta)
 * @return Handle da tarefa ou 0 se erro
 */
rtos_task_handle_t rtos_task_create(
    rtos_task_function_t func,
    const char *name,
    size_t stack_size,
    void *param,
    uint8_t priority
);

/**
 * @brief Deleta uma tarefa
 * @param task_handle Handle da tarefa
 * @return RTOS_OK em caso de sucesso
 */
int rtos_task_delete(rtos_task_handle_t task_handle);

/**
 * @brief Coloca a tarefa atual em sleep
 * @param ms Tempo em milissegundos
 */
void rtos_task_sleep(uint32_t ms);

/**
 * @brief Cede o processador para outra tarefa
 */
void rtos_task_yield(void);

/**
 * @brief Obtém a prioridade atual da tarefa
 * @param task_handle Handle da tarefa
 * @return Prioridade (0-31)
 */
uint8_t rtos_task_get_priority(rtos_task_handle_t task_handle);

/**
 * @brief Define a prioridade da tarefa
 * @param task_handle Handle da tarefa
 * @param priority Nova prioridade (0-31)
 * @return RTOS_OK em caso de sucesso
 */
int rtos_task_set_priority(rtos_task_handle_t task_handle, uint8_t priority);

/**
 * @brief Obtém o estado atual da tarefa
 * @param task_handle Handle da tarefa
 * @return Estado da tarefa
 */
rtos_task_state_t rtos_task_get_state(rtos_task_handle_t task_handle);

/**
 * @brief Obtém a tarefa em execução
 * @return Handle da tarefa atual
 */
rtos_task_handle_t rtos_task_get_current(void);

/* ============================================
   API de Semáforos
   ============================================ */

/**
 * @brief Cria um novo semáforo
 * @param initial_count Contagem inicial
 * @return Handle do semáforo ou 0 se erro
 */
rtos_semaphore_t rtos_semaphore_create(uint32_t initial_count);

/**
 * @brief Aguarda um semáforo (decrementa)
 * @param sem Handle do semáforo
 * @param timeout_ms Timeout em ms (0 = infinito)
 * @return RTOS_OK se sucesso, RTOS_TIMEOUT se timeout
 */
int rtos_semaphore_take(rtos_semaphore_t sem, uint32_t timeout_ms);

/**
 * @brief Libera um semáforo (incrementa)
 * @param sem Handle do semáforo
 * @return RTOS_OK em caso de sucesso
 */
int rtos_semaphore_give(rtos_semaphore_t sem);

/**
 * @brief Deleta um semáforo
 * @param sem Handle do semáforo
 * @return RTOS_OK em caso de sucesso
 */
int rtos_semaphore_delete(rtos_semaphore_t sem);

/* ============================================
   API de Mutexes
   ============================================ */

/**
 * @brief Cria um novo mutex
 * @return Handle do mutex ou 0 se erro
 */
rtos_mutex_t rtos_mutex_create(void);

/**
 * @brief Bloqueia um mutex
 * @param mtx Handle do mutex
 * @param timeout_ms Timeout em ms (0 = infinito)
 * @return RTOS_OK se sucesso, RTOS_TIMEOUT se timeout
 */
int rtos_mutex_lock(rtos_mutex_t mtx, uint32_t timeout_ms);

/**
 * @brief Desbloqueia um mutex
 * @param mtx Handle do mutex
 * @return RTOS_OK em caso de sucesso
 */
int rtos_mutex_unlock(rtos_mutex_t mtx);

/**
 * @brief Deleta um mutex
 * @param mtx Handle do mutex
 * @return RTOS_OK em caso de sucesso
 */
int rtos_mutex_delete(rtos_mutex_t mtx);

/* ============================================
   API de Filas
   ============================================ */

/**
 * @brief Cria uma nova fila
 * @param item_size Tamanho de cada item em bytes
 * @param queue_length Número máximo de itens
 * @return Handle da fila ou 0 se erro
 */
rtos_queue_t rtos_queue_create(size_t item_size, size_t queue_length);

/**
 * @brief Envia uma mensagem para a fila
 * @param queue Handle da fila
 * @param item Ponteiro para o item a enviar
 * @param timeout_ms Timeout em ms (0 = não espera)
 * @return RTOS_OK se sucesso
 */
int rtos_queue_send(rtos_queue_t queue, const void *item, uint32_t timeout_ms);

/**
 * @brief Recebe uma mensagem da fila
 * @param queue Handle da fila
 * @param item Ponteiro para buffer que receberá o item
 * @param timeout_ms Timeout em ms (0 = infinito)
 * @return RTOS_OK se sucesso, RTOS_TIMEOUT se timeout
 */
int rtos_queue_receive(rtos_queue_t queue, void *item, uint32_t timeout_ms);

/**
 * @brief Deleta uma fila
 * @param queue Handle da fila
 * @return RTOS_OK em caso de sucesso
 */
int rtos_queue_delete(rtos_queue_t queue);

/* ============================================
   API de Memória
   ============================================ */

/**
 * @brief Aloca memória do heap RTOS
 * @param size Tamanho em bytes
 * @return Ponteiro para memória alocada ou NULL
 */
void *rtos_malloc(size_t size);

/**
 * @brief Libera memória do heap RTOS
 * @param ptr Ponteiro para liberar
 */
void rtos_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_H */
