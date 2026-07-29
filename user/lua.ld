ENTRY(_start)

SECTIONS
{
  . = DEFINED(USER_BASE) ? USER_BASE : 0x50000000;

  .text : {
    *(.text .text.*)
    *(.rodata .rodata.*)
  }

  .data : {
    *(.data .data.*)
  }

  .bss (NOLOAD) : {
    *(.bss .bss.*)
    *(COMMON)
  }
}
