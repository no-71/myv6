# set confirm off
set architecture riscv:rv64
symbol-file out/kernel
file out/kernel
target remote localhost:26000
# set disassemble-next-line auto
# set riscv use-compressed-breakpoints yes
