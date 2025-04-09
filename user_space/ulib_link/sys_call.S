.text

.global kernelbreak
kernelbreak:
    li a7, 0
    ecall
    ret

.global sleep
sleep:
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

.global time
time:
    li a7, 10
    ecall
    ret

.global dup
dup:
    li a7, 11
    ecall
    ret

.global read
read:
    li a7, 12
    ecall
    ret

.global write
write:
    li a7, 13
    ecall
    ret

.global close
close:
    li a7, 14
    ecall
    ret

.global fstat
fstat:
    li a7, 15
    ecall
    ret

.global link
link:
    li a7, 16
    ecall
    ret

.global unlink
unlink:
    li a7, 17
    ecall
    ret

.global open
open:
    li a7, 18
    ecall
    ret

.global mkdir
mkdir:
    li a7, 19
    ecall
    ret

.global mknod
mknod:
    li a7, 20
    ecall
    ret

.global chdir
chdir:
    li a7, 21
    ecall
    ret

.global pipe
pipe:
    li a7, 22
    ecall
    ret

.global get_proc_group_id
get_proc_group_id:
    li a7, 100
    ecall
    ret

.global create_proc_group
create_proc_group:
    li a7, 101
    ecall
    ret

.global enter_proc_group
enter_proc_group:
    li a7, 102
    ecall
    ret

.global proc_group_cpus
proc_group_cpus:
    li a7, 103
    ecall
    ret

.global inc_proc_group_cpus
inc_proc_group_cpus:
    li a7, 104
    ecall
    ret

.global dec_proc_group_cpus
dec_proc_group_cpus:
    li a7, 105
    ecall
    ret

.global proc_group_procs_count
proc_group_procs_count:
    li a7, 106
    ecall
    ret

.global proc_occupy_cpu
proc_occupy_cpu:
    li a7, 107
    ecall
    ret

.global proc_release_cpu
proc_release_cpu:
    li a7, 108
    ecall
    ret

.global inc_proc_group_cpus_flex
inc_proc_group_cpus_flex:
    li a7, 109
    ecall
    ret

.global count_proc_num
count_proc_num:
    li a7, 1000
    ecall
    ret
