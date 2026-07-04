[bits 32]

global task_switch
global task_bootstrap

; task_switch(uint32_t *save_esp, uint32_t new_esp)
;
; O truque inteiro da multitarefa está aqui: cada tarefa tem sua própria
; stack. Salvamos os registradores na stack ATUAL, guardamos o ESP,
; trocamos para o ESP da outra tarefa e restauramos os registradores DELA.
; O `ret` então "volta" para onde ELA estava — não para onde nós estávamos.
;
; Só salvamos edi/esi/ebx/ebp (callee-saved): os demais o compilador já
; considera perdidos em qualquer chamada de função. EIP fica implícito:
; é o endereço de retorno que o `call task_switch` empilhou.
task_switch:
    mov eax, [esp+4]    ; eax = &tarefa_atual->esp
    mov edx, [esp+8]    ; edx = esp da próxima tarefa
    push ebp
    push ebx
    push esi
    push edi
    mov [eax], esp      ; congela a tarefa atual
    mov esp, edx        ; troca de stack — daqui em diante somos a outra tarefa
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret                 ; salta para onde a nova tarefa parou

; Primeira execução de uma tarefa nova: a stack forjada faz o `ret` do
; task_switch cair aqui. Liga interrupções (estamos vindo de dentro de um
; handler de IRQ, onde IF=0) e "retorna" para a função da tarefa.
task_bootstrap:
    sti
    ret
