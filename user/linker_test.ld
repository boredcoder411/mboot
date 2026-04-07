ENTRY(_start)

SECTIONS  
{
  /* Force symbols to known addresses - calculated from libc offsets */
  put_pixel         = 0x40000000;
  clear_screen      = 0x40000030;
  sleep_ms          = 0x40000060;
  read_key          = 0x40000040;
  has_key           = 0x40000080;
  memcpy            = 0x400000a0;
  memset            = 0x400000f0;
  strlen            = 0x40000190;
  strcmp            = 0x400001c0;
  strcpy            = 0x40000140;
  draw_square       = 0x40000290;
  srand             = 0x40000280;
  rand              = 0x40000250;
  draw_rect         = 0x40000320;
  draw_line         = 0x400003c0;
  draw_circle       = 0x400004d0;
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



