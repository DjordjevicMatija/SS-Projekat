.global main
.extern test
.section text
.skip 0x10
main:
    add %r2, %r3
    .word 10, 20, 30
    .end

