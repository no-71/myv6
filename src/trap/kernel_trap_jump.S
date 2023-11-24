.text
.global kernel_trap_entry
kernel_trap_entry:
    sret
    call kernel_trap_handler
