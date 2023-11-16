.text
.global trap_into_k

trap_into_k:
    li a0, 0
    ecall
    ret
