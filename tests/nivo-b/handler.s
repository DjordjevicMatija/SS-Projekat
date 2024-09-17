.extern isr_timer, isr_terminal

.global handler
.section my_handler
handler:
    push %r1
    push %r2
    csrrd %cause, %r1
    ld $2, %r2
    beq %r1, %r2, handle_timer
    ld $3, %r2
    beq %r1, %r2, handle_terminal
finish:
    pop %r2
    pop %r1
    iret
# obrada prekida od tajmera
handle_timer:
    call isr_timer
    jmp finish
# obrada prekida od terminala
handle_terminal:
    call isr_terminal
    jmp finish
    
.end
