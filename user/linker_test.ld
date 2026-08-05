ENTRY(_start)

SECTIONS  
{
  /* Force symbols to known addresses - calculated from libc offsets */
  memcpy            = 0x400000a0;
  memset            = 0x400000f0;
  strlen            = 0x40000190;
  strcmp            = 0x400001c0;
  strcpy            = 0x40000140;
  srand             = 0x40000280;
  rand              = 0x40000250;
  malloc            = 0x400007b0;
  free              = 0x40000800;
  
  . = 0x50000000;
  
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



