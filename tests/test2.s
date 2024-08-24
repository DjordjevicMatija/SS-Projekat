# file: main.s

.extern handler, mathAdd, mathSub, mathMul, mathDiv

.global my_start

.global value1, value2, value3, value4, value5, value6, value7

.section my_code
my_start:
  int
  iret 
  call handler
  ret
  jmp mathAdd
  beq %r1, %r2, 0xF0000000
  bne %r1, %r2, 0xF0000000
  bgt %r1, %r2, 50
  push %r1
  pop %r1
  xchg %r1, %r2
  add %r1, %r2
  sub %r1, %r2
  mul %r1, %r2
  div %r1, %r2
  not %r1
  and %r1, %r2
  or %r1, %r2
  xor %r1, %r2
  shl %r1, %r2
  shr %r1, %r2
  csrrd %handler, %r1
  csrwr %r1, %handler
  ld $0xFFFFFEFE, %sp
  ld $handler, %r1
  ld 0xF0000000, %r1
  ld banger, %r1
  ld %r2, %r1
  ld [%r2], %r1
  ld [%r2 + 4], %r1
#  ld [%r2 + my_start], %r1

  st %r1, 0xF0000000
  st %r1, banger
  st %r1, [%r2]
  st %r1, [%r2 + 4]
#  st %r1, [%r2 + my_start]


  halt


.section my_data

banger:
.skip 4
value1:
.word 0
value2:
.word banger
value3:
.word 0
value4:
.word 0
value5:
.word banger
value6:
.word 0
value7:
.word 0

.end