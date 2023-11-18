OUTPUT_ARCH("riscv")
ENTRY(user_start)

SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   */
  . = 0x1000;

  .text : {
    *(the_user_start)
    *(.text .text.*)
    . = ALIGN(0x1000);
  }

  .data : {
    . = ALIGN(16);
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(16);
    *(.bss .bss.*)
  }
}
