.section .text
.global _entry
_entry:
    call main
spin:
    j spin
