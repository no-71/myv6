.global time_trap_vec
time_trap_vec:
    csrrw a0, mscratch, a0
    sd a1, 24(a0)
    sd a2, 32(a0)
    sd a3, 40(a0)

    # a1 = mtimecmp, a2 = interval
    ld a1, 0(a0)
    ld a2, 16(a0)
    # a3 = last timecmp
    ld a3, 0(a1)
    # next trap time
    add a3, a3, a2
    sd a3, 0(a1)

    # raise software intrrupt
    csrr a1, sip
    addi a1, a1, 2
    csrw sip, a1

    # finish, ret
    ld a1, 24(a0)
    ld a2, 32(a0)
    ld a3, 40(a0)
    csrrw a0, mscratch, a0
    mret
