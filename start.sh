ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

${ASSEMBLER} -o main.o ./tests/main.s
${ASSEMBLER} -o math.o ./tests/math.s
${ASSEMBLER} -o handler.o ./tests/handler.s
${ASSEMBLER} -o isr_timer.o ./tests/isr_timer.s
${ASSEMBLER} -o isr_terminal.o ./tests/isr_terminal.s
${ASSEMBLER} -o isr_software.o ./tests/isr_software.s
${LINKER} -hex \
  -place=my_code@0x40000000 -place=math@0xF0000000 \
  -o program.hex \
  handler.o math.o main.o isr_terminal.o isr_timer.o isr_software.o
${EMULATOR} program.hex