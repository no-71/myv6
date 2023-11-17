# set confirm off
set architecture riscv:rv64
symbol-file out/kernel
file out/kernel
b painc_2str
b user_trap_ret
target remote localhost:26000
c
del 1
# set disassemble-next-line auto
# set riscv use-compressed-breakpoints yes
