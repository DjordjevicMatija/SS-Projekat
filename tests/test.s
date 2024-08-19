.global main
.section text
main:
    add %r2, %r3
    .word 10, 20, 30
    .skip 100
    .end

