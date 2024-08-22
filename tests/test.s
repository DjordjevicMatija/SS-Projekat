.global main
.extern test
.section text
.skip 0x10
main:
    halt
    int
    xchg %r4, %r5
    add %r2, %r1
proba:
    .word 10, 20, 30
    or %r7, %r2
    shl %r3, %r13
    csrwr %r2, %HANDLER
    .end

