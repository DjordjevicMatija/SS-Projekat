.extern h1
.global main
.section text
main:
    ld $0xFFFFFFFE, %sp
    ld $h1, %r1
    ld $0xFFFFFFFF, %r1
    jmp main
main2:
    .end