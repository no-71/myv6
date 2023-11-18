.text

.global putc
putc:
    li a7, 0
    ecall
    ret

.global puts
puts:
    li a7, 1
    ecall
    ret
