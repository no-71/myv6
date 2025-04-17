# set confirm off
set architecture riscv:rv64
symbol-file out/_kernel
file out/_kernel
target remote localhost:26000
# set disassemble-next-line auto
# set riscv use-compressed-breakpoints yes
