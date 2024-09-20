ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

${ASSEMBLER} -o main.o ./tests/nivo-b/main.s
${ASSEMBLER} -o handler.o ./tests/nivo-b/handler.s
${ASSEMBLER} -o isr_terminal.o ./tests/nivo-b/isr_terminal.s
${ASSEMBLER} -o isr_timer.o ./tests/nivo-b/isr_timer.s
${LINKER} -hex \
  -place=my_code@0x40000000 \
  -o program.hex \
  main.o isr_terminal.o isr_timer.o handler.o
${EMULATOR} program.hex