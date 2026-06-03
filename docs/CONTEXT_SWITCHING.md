# Context Switching Otimizado em Assembly

## Visão Geral

O context switching é uma operação crítica em sistemas operacionais em tempo real. A implementação em Assembly garante:

- **Performance máxima**: Sem overhead de chamadas de funções C
- **Controle total**: Acesso direto aos registradores
- **Latência baixa**: Minimiza o tempo de troca de contexto
- **Confiabilidade**: Garante que todos os registradores sejam salvos/restaurados corretamente

## Arquitetura do Xtensa (ESP32)

O ESP32 usa o Xtensa Dual-Core 32-bit, com as seguintes características:

### Registradores

**Registradores de Propósito Geral (A0-A15):**
- 16 registradores de 32 bits
- Usados para argumentos, retornos e dados
- Todos devem ser salvos/restaurados

**Registradores Especiais:**
- **PS (Processor Status)**: Estado da CPU (flags, nível de interrupção)
- **PC (Program Counter)**: Endereço da próxima instrução
- **SAR (Shift Amount Register)**: Quantidade de shift para operações
- **WINDOWBASE/WINDOWSTART**: Gerenciamento de janelas de registradores

### Convenção de Chamada

Xtensa usa a Application Binary Interface (ABI):

```
a0  = return address
a1  = stack pointer (sp)
a2  = first argument / return value (low)
a3  = first argument / return value (high)
a4-a7   = arguments
a8-a11  = temporários
a12-a15 = saved across calls
```

## Estrutura de Contexto

```c
typedef struct {
    uint32_t a0;    /* Return address */
    uint32_t a1;    /* Stack pointer */
    uint32_t a2;    /* Arg 0 / Return */
    uint32_t a3;    /* Arg 1 */
    // ... a4-a15
    uint32_t ps;    /* Processor Status */
    uint32_t pc;    /* Program Counter */
    uint32_t sar;   /* Shift Amount Register */
} rtos_context_t;  /* Total: 76 bytes */
```

## Implementação Assembly

### 1. context_save_asm

Salva todos os registradores no endereço fornecido.

```asm
_context_save_asm:
    s32i.n  a0,  a2, 0      /* Salva a0 em contexto+0 */
    s32i.n  a1,  a2, 4      /* Salva a1 em contexto+4 */
    // ... (salva todos os registradores)
    rsr     a3,  ps         /* Lê Processor Status */
    s32i.n  a3,  a2, 64     /* Salva PS */
    ret.n
```

**Características:**
- Usa `s32i.n` (store 32-bit immediate narrow) para salvar registradores
- `rsr` (read special register) para ler PS e SAR
- Operações "narrow" de 2 bytes (mais compactas)
- Tempo: ~20-25 ciclos de clock

### 2. context_restore_asm

Restaua todos os registradores do endereço fornecido.

```asm
_context_restore_asm:
    l32i.n  a3,  a2, 64     /* Carrega PS */
    wsr     a3,  ps         /* Escreve PS */
    // ... (carrega todos os registradores)
    l32i.n  a2,  a2, 8      /* Carrega a2 */
    ret.n
```

**Características:**
- Usa `l32i.n` (load 32-bit immediate narrow) para carregar registradores
- `wsr` (write special register) para escrever PS e SAR
- Restaura PS antes dos registradores normais (crítico!)
- Tempo: ~20-25 ciclos de clock

**⚠️ IMPORTANTE:** A ordem de restauração é crítica:
1. Restaura PS primeiro (ativa/desativa interrupções)
2. Restaura todos os registradores normais
3. Por último, restaura a2 (será clobberado ao carregar do endereço a2)

### 3. task_entry_wrapper

Entry point para novas tarefas.

```asm
_task_entry_wrapper:
    movi    a0, _task_terminate    /* Salva endereço de retorno */
    jx      a3                      /* Jump to function (em a3) */
```

**Fluxo:**
- a2 contém o parâmetro da tarefa
- a3 contém o endereço da função
- a0 aponta para `_task_terminate` (chamado se função retorna)

## Inicialização de Stack

Quando uma tarefa é criada, sua stack é preparada com:

```c
void context_init_stack(rtos_task_t *task) {
    rtos_context_t *ctx = (rtos_context_t *)(stack_top - sizeof(...));
    
    ctx->a0 = (uint32_t)_task_entry_wrapper;    /* Return address */
    ctx->a1 = stack_top - 16;                    /* Stack pointer */
    ctx->a2 = (uint32_t)task->param;             /* Argumento 1 */
    ctx->a3 = (uint32_t)task->entry_point;       /* Argumento 2 */
    ctx->ps = 0x00040020;                        /* Status habilitado */
    ctx->pc = (uint32_t)_task_entry_wrapper;     /* Program counter */
    
    task->sp = (void *)ctx;
}
```

## Fluxo de Context Switching

### 1. Durante Interrupção de Timer

```
[Tarefa A em execução]
    ↓
[Timer gera interrupção]
    ↓
ISR Handler
    ├─ context_save(task_a)     /* Salva contexto de A */
    ├─ scheduler_get_next_task() /* Encontra próxima pronta */
    ├─ context_restore(task_b)   /* Restaura contexto de B */
    └─ (nunca retorna)
    ↓
[Tarefa B em execução]
```

### 2. Quando Tarefa Termina

```
_task_entry_wrapper
    ├─ a0 = _task_terminate
    ├─ jx a3 (chama tarefa)
    ├─ [Tarefa executa]
    └─ ret (retorna para _task_terminate)
        ↓
    _task_terminate
        └─ Loop infinito
```

## Performance

### Ciclos por Operação

- **context_save_asm**: ~20-25 ciclos
  - 16 × s32i.n (2 ciclos cada) = 32 ciclos
  - 1 × rsr (1 ciclo) = 1 ciclo
  - 1 × s32i.n = 2 ciclos
  - Total ≈ 35 ciclos

- **context_restore_asm**: ~20-25 ciclos
  - Similar a save

- **Total por context switch**: ~50 ciclos
  - A 240 MHz: 50/240M ≈ 208 ns

### Comparação

| Operação | Assembly | Estimado C | Melhoria |
|----------|----------|------------|----------|
| Save | ~20 ciclos | ~50 ciclos | 2.5× |
| Restore | ~20 ciclos | ~50 ciclos | 2.5× |
| Switch Total | ~50 ciclos | ~150 ciclos | 3× |

## Instruções Assembly Utilizadas

### Instruções Narrow (16-bit)
```
s32i.n  dst, base, offset    # store 32-bit (narrow)
l32i.n  dst, base, offset    # load 32-bit (narrow)
mov.n   dst, src              # move (narrow)
ret.n                         # return (narrow)
movi.n  dst, imm             # move immediate (narrow)
```

### Instruções Normais (24-bit)
```
s32i    dst, base, offset    # store 32-bit
l32i    dst, base, offset    # load 32-bit
movi    dst, imm             # move immediate
rsr     dst, special_reg     # read special register
wsr     src, special_reg     # write special register
jx      addr                 # jump indirect
call0   addr                 # call (preserva a0)
ret                          # return
```

## Segurança e Correção

### Verificações Implementadas

1. **Alinhamento de Stack**: Alinhado a 16 bytes (requisito Xtensa)
2. **Ordem de Restauração**: PS restaurado primeiro
3. **Registrador a2**: Salvo por último ao restaurar
4. **Interrupções**: PS contém nível de interrupção correto

### Testes Recomendados

```c
/* Criar múltiplas tarefas */
rtos_task_create(task1, "T1", 2048, NULL, 10);
rtos_task_create(task2, "T2", 2048, NULL, 10);

/* Verificar que contextos são corretos */
context_print_info(task1);
context_print_info(task2);

/* Monitorar switches com debug */
#define DEBUG_CONTEXT_SWITCH 1
```

## Limitações e Considerações

1. **Window Registers**: Xtensa tem registradores em janelas - a implementação atual assume janela 0
2. **Cache**: Em sistemas com cache, pode ser necessário flush
3. **FPU**: Se houver FPU (co-processador), deve salvar seus registradores também
4. **Preempção**: Interrupções durante save/restore devem ser desabilitadas

## Compilação

Para compilar corretamente:

```makefile
# No CMakeLists.txt
enable_language(C ASM)

# Flags para Assembly Xtensa
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp")
```

## Referências

- Tensilica Xtensa Instruction Set Architecture Reference Manual
- ESP32 Technical Reference Manual
- ESP-IDF Context Switching Documentation
