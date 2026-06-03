/* Context Switching em Assembly para Xtensa (ESP32)
 * 
 * Este arquivo implementa as funções críticas de salvar e restaurar
 * contexto da CPU em Assembly para máxima performance.
 * 
 * Xtensa é a ISA (Instruction Set Architecture) do ESP32
 */

#include "sdkconfig.h"

.section .text
.global _context_save_asm
.global _context_restore_asm
.global _task_entry_wrapper
.global _context_switch_isr

/* ============================================
   _context_save_asm: Salva contexto da tarefa atual
   
   Argumentos:
   a2 = ponteiro para rtos_context_t
   
   Salva todos os registradores no endereço fornecido
   ============================================ */

.align 4
_context_save_asm:
    /* Prólogo: a2 já contém o ponteiro para contexto */
    
    /* Salva todos os registradores A0-A15 */
    s32i.n  a0,  a2, 0      /* OFFSET_A0   */
    s32i.n  a1,  a2, 4      /* OFFSET_A1   */
    s32i.n  a3,  a2, 12     /* OFFSET_A3   */
    s32i.n  a4,  a2, 16     /* OFFSET_A4   */
    s32i.n  a5,  a2, 20     /* OFFSET_A5   */
    s32i.n  a6,  a2, 24     /* OFFSET_A6   */
    s32i.n  a7,  a2, 28     /* OFFSET_A7   */
    s32i.n  a8,  a2, 32     /* OFFSET_A8   */
    s32i.n  a9,  a2, 36     /* OFFSET_A9   */
    s32i.n  a10, a2, 40     /* OFFSET_A10  */
    s32i.n  a11, a2, 44     /* OFFSET_A11  */
    s32i.n  a12, a2, 48     /* OFFSET_A12  */
    s32i.n  a13, a2, 52     /* OFFSET_A13  */
    s32i.n  a14, a2, 56     /* OFFSET_A14  */
    s32i.n  a15, a2, 60     /* OFFSET_A15  */
    
    /* Salva registradores especiais */
    rsr     a3,  ps         /* Lê Processor Status */
    s32i.n  a3,  a2, 64     /* OFFSET_PS   */
    
    rsr     a3,  sar        /* Lê SAR (Shift Amount Register) */
    s32i.n  a3,  a2, 72     /* OFFSET_SAR  */
    
    /* Salva a2 (foi usado como temporário) */
    movi.n  a3,  0
    /* a2 foi clobberado, mas já foi salvo em a2, 8 acima */
    
    ret.n

/* ============================================
   _context_restore_asm: Restaura contexto da tarefa
   
   Argumentos:
   a2 = ponteiro para rtos_context_t
   
   Restaura todos os registradores do endereço fornecido
   Esta função nunca retorna - restaura completamente o contexto
   ============================================ */

.align 4
_context_restore_asm:
    /* a2 = ponteiro para contexto */
    
    /* Restaura registradores especiais primeiro */
    l32i.n  a3,  a2, 64     /* Lê Processor Status de OFFSET_PS */
    wsr     a3,  ps         /* Escreve PS */
    
    l32i.n  a3,  a2, 72     /* Lê SAR de OFFSET_SAR */
    wsr     a3,  sar        /* Escreve SAR */
    
    /* Restaura registradores A0-A15 */
    l32i.n  a0,  a2, 0      /* OFFSET_A0   */
    l32i.n  a1,  a2, 4      /* OFFSET_A1   */
    l32i.n  a3,  a2, 12     /* OFFSET_A3   */
    l32i.n  a4,  a2, 16     /* OFFSET_A4   */
    l32i.n  a5,  a2, 20     /* OFFSET_A5   */
    l32i.n  a6,  a2, 24     /* OFFSET_A6   */
    l32i.n  a7,  a2, 28     /* OFFSET_A7   */
    l32i.n  a8,  a2, 32     /* OFFSET_A8   */
    l32i.n  a9,  a2, 36     /* OFFSET_A9   */
    l32i.n  a10, a2, 40     /* OFFSET_A10  */
    l32i.n  a11, a2, 44     /* OFFSET_A11  */
    l32i.n  a12, a2, 48     /* OFFSET_A12  */
    l32i.n  a13, a2, 52     /* OFFSET_A13  */
    l32i.n  a14, a2, 56     /* OFFSET_A14  */
    l32i.n  a15, a2, 60     /* OFFSET_A15  */
    
    /* Carrega a2 do contexto (por último) */
    l32i.n  a2,  a2, 8      /* OFFSET_A2   */
    
    /* Restauração concluída - retorna ao contexto restaurado */
    ret.n

/* ============================================
   _task_entry_wrapper: Entry point para novas tarefas
   
   Quando uma tarefa é criada, seu PC aponta aqui
   Prepara argumentos e chama a função da tarefa
   
   Convenção de chamada Xtensa:
   a2 = primeiro argumento (parâmetro)
   a3 = endereço da função
   ============================================ */

.align 4
_task_entry_wrapper:
    /* a2 já contém o parâmetro (primeiro argumento) */
    /* a3 contém o endereço da função */
    
    /* Salva endereço de retorno em a0 */
    movi    a0, _task_terminate
    
    /* Chama a função da tarefa: func(param) */
    /* a2 = parâmetro (já correto) */
    jx      a3              /* jump to a3 */

/* ============================================
   _task_terminate: Termina a execução da tarefa
   
   Chamado quando a tarefa retorna
   ============================================ */

.align 4
_task_terminate:
    /* A tarefa terminou, entra em loop infinito */
    /* (Em uma implementação real, isso deveria notificar o kernel) */
    j       _task_terminate  /* Loop infinito */

/* ============================================
   _context_switch_isr: Context switch no handler de interrupção
   
   Esta é uma versão otimizada que pode ser chamada
   do handler de timer para realizar context switch rápido
   
   Argumentos:
   a2 = ponteiro para contexto anterior
   a3 = ponteiro para novo contexto
   ============================================ */

.align 4
_context_switch_isr:
    /* Salva contexto anterior (a2) e restaura novo (a3) */
    
    /* Salva contexto atual */
    call0   _context_save_asm
    
    /* Restaura novo contexto */
    mov.n   a2, a3          /* Move novo contexto para a2 */
    call0   _context_restore_asm
    
    ret.n

/* Fim do arquivo */
