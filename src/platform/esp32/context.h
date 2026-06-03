#ifndef CONTEXT_H
#define CONTEXT_H

#include "../../../src/scheduler/task.h"
#include <stdint.h>

/* ============================================
   API de Context Switching
   ============================================ */

/**
 * @brief Salva contexto da tarefa atual
 * @param task Ponteiro para estrutura da tarefa
 * 
 * Salva todos os registradores (A0-A15) e registradores
 * especiais (PS, SAR) no contexto da tarefa
 */
void context_save(rtos_task_t *task);

/**
 * @brief Restaura contexto da tarefa
 * @param task Ponteiro para estrutura da tarefa
 * 
 * Restaura todos os registradores e retorna à execução
 * Esta função nunca retorna normalmente
 */
void context_restore(rtos_task_t *task);

/**
 * @brief Inicializa stack de uma nova tarefa
 * @param task Ponteiro para estrutura da tarefa
 * 
 * Prepara a stack com o contexto inicial para que
 * a tarefa possa começar a executar
 */
void context_init_stack(rtos_task_t *task);

/**
 * @brief Imprime informações de debug do contexto
 * @param task Ponteiro para estrutura da tarefa
 */
void context_print_info(rtos_task_t *task);

#endif /* CONTEXT_H */
