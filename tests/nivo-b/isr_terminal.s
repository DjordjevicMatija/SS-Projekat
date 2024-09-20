# file: isr_terminal.s

.extern my_counter

.section isr
# prekidna rutina za terminal
.global isr_terminal
isr_terminal:
    push %r1
    push %r2

    ld 0xFFFFFF04, %r1 # term_in
    ld $121, %r2 # character_offset
    bne %r1, %r2, skip

    ld $message_start, %r1

    ld [%r1+0], %r2
    st %r2, 0xFFFFFF00 # term_out
    ld [%r1+1], %r2
    st %r2, 0xFFFFFF00 # term_out
    ld [%r1+2], %r2
    st %r2, 0xFFFFFF00 # term_out
    ld [%r1+3], %r2
    st %r2, 0xFFFFFF00 # term_out
    ld [%r1+4], %r2
    st %r2, 0xFFFFFF00 # term_out

    ld my_counter, %r1 
    ld $1, %r2
    add %r2, %r1
    st %r1, my_counter

skip:
    pop %r2
    pop %r1

    ret
message_start:
.ascii "press"

.end
