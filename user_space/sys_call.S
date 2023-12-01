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

.global sbrk
sbrk:
    li a7, 2
    ecall
    ret

.global fork
fork:
    li a7, 3
    ecall
    ret

.global exec
exec:
    li a7, 4
    ecall
    ret

.global getpid
getpid:
    li a7, 5
    ecall
    ret

.global exit
exit:
    li a7, 6
    ecall
    ret

.global wait
wait:
    li a7, 7
    ecall
    ret

.global kill
kill:
    li a7, 8
    ecall
    ret

.global brk
brk:
    li a7, 9
    ecall
    ret

.global getc
getc:
    li a7, 10
    ecall
    ret

.global count_proc_num
count_proc_num:
    li a7, 1000
    ecall
    ret
