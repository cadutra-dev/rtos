#include "../../../include/rtos.h"
#include "../../../src/scheduler/task.h"
#include <stdio.h>
#include <string.h>

/* ============================================
   Estrutura de Contexto (Xtensa)
   ============================================ */

/* Definições de offsets para acesso em assembly */
#define CONTEXT_SIZE    76  /* Tamanho da estrutura em bytes */
#define OFFSET_A0       0   /* a0 (return address) */
#define OFFSET_A1       4   /* a1 (stack pointer) */
#define OFFSET_A2       8   /* a2 */
#define OFFSET_A3       12  /* a3 */
#define OFFSET_A4       16  /* a4 */
#define OFFSET_A5       20  /* a5 */
#define OFFSET_A6       24  /* a6 */
#define OFFSET_A7       28  /* a7 */
#define OFFSET_A8       32  /* a8 */
#define OFFSET_A9       36  /* a9 */
#define OFFSET_A10      40  /* a10 */
#define OFFSET_A11      44  /* a11 */
#define OFFSET_A12      48  /* a12 */
#define OFFSET_A13      52  /* a13 */
#define OFFSET_A14      56  /* a14 */
#define OFFSET_A15      60  /* a15 */
#define OFFSET_PS       64  /* Processor Status */
#define OFFSET_PC       68  /* Program Counter (retorno) */
#define OFFSET_SAR      72  /* SAR (Shift Amount Register) */

/* Estrutura para armazenar contexto da tarefa */
typedef struct {
    uint32_t a0;    /* Registrador A0 */
    uint32_t a1;    /* Stack pointer */
    uint32_t a2;    /* Argumentos e retorno */
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t a8;
    uint32_t a9;
    uint32_t a10;
    uint32_t a11;
    uint32_t a12;
    uint32_t a13;
    uint32_t a14;
    uint32_t a15;
    uint32_t ps;    /* Processor Status */
    uint32_t pc;    /* Program Counter */
    uint32_t sar;   /* Shift Amount Register */
} rtos_context_t;

/* ============================================
   Funções externas implementadas em Assembly
   ============================================ */

extern void _context_save_asm(rtos_context_t *ctx);
extern void _context_restore_asm(rtos_context_t *ctx);
extern void _task_entry_wrapper(void);

/* ============================================
   API Pública
   ============================================ */

void context_save(rtos_task_t *task) {
    if (task == NULL) {
        return;
    }

    /* Obtém endereço da estrutura de contexto da tarefa */
    rtos_context_t *ctx = (rtos_context_t *)task->sp;
    
    /* Chama função em assembly para salvar registradores */
    _context_save_asm(ctx);
}

void context_restore(rtos_task_t *task) {
    if (task == NULL) {
        return;
    }

    /* Obtém endereço da estrutura de contexto da tarefa */
    rtos_context_t *ctx = (rtos_context_t *)task->sp;
    
    /* Chama função em assembly para restaurar registradores */
    _context_restore_asm(ctx);
    
    /* context_restore_asm nunca retorna - restaura o contexto completamente */
}

/* ============================================
   Inicialização de Stack de Tarefa
   ============================================ */

void context_init_stack(rtos_task_t *task) {
    if (task == NULL || task->stack == NULL) {
        return;
    }

    /* Alinha stack para 16 bytes (requisito do Xtensa) */
    uintptr_t stack_top = ((uintptr_t)task->stack + task->stack_size) & ~15;
    
    /* Cria estrutura de contexto no topo da stack */
    rtos_context_t *ctx = (rtos_context_t *)(stack_top - sizeof(rtos_context_t));
    
    /* Inicializa contexto */
    memset(ctx, 0, sizeof(rtos_context_t));
    
    /* a0: endereço de retorno (para quando tarefa terminar) */
    ctx->a0 = (uint32_t)_task_entry_wrapper;
    
    /* a1: stack pointer (deve apontar acima do contexto) */
    ctx->a1 = stack_top - 16;  /* Deixa espaço para stack frame */
    
    /* a2: primeiro argumento (parâmetro da tarefa) */
    ctx->a2 = (uint32_t)task->param;
    
    /* a3: segundo argumento (função de entrada) */
    ctx->a3 = (uint32_t)task->entry_point;
    
    /* ps: Processor Status - modo user, interrupções ativadas */
    ctx->ps = 0x00040020;  /* INTLEVEL=0, EXCM=0 */
    
    /* pc: Program Counter - apontará para entry point */
    ctx->pc = (uint32_t)_task_entry_wrapper;
    
    /* sar: Shift Amount Register - zerado */
    ctx->sar = 0;
    
    /* Armazena ponteiro para contexto na tarefa */
    task->sp = (void *)ctx;
}

/* ============================================
   Informações de Debug
   ============================================ */

void context_print_info(rtos_task_t *task) {
    if (task == NULL) {
        return;
    }

    rtos_context_t *ctx = (rtos_context_t *)task->sp;
    
    printf("\n=== Contexto da Tarefa %s ===", task->name);
    printf("\nStack Pointer (a1): 0x%08x", ctx->a1);
    printf("\nProgram Counter:    0x%08x", ctx->pc);
    printf("\nProcessor Status:   0x%08x", ctx->ps);
    printf("\na0 (retorno):       0x%08x", ctx->a0);
    printf("\na2 (arg0/ret):      0x%08x", ctx->a2);
    printf("\na3 (arg1):          0x%08x", ctx->a3);
    printf("\na4 (arg2):          0x%08x", ctx->a4);
    printf("\nSAR:                0x%08x\n", ctx->sar);
}
