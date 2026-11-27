ENTRY(reset)
MEMORY {
  CODE (rx) : org = 0x10000000, len = 4M
  RAM (rwx) : org = 0x80000000, len = 16M
}
SECTIONS {
  .vectors : { KEEP(*(.vectors)) } > CODE
  .text : { *(.text* .rodata*) } > CODE
  .data : { __data_start = .; *(.data*) . = ALIGN(4); __data_end = .; } > RAM
  __data_load = LOADADDR(.data);
  .bss (NOLOAD) : { __bss_start = .; *(.bss* COMMON) . = ALIGN(4); __bss_end = .; } > RAM
  .heap (NOLOAD) : ALIGN(8) { __heap_start = .; . += 1M; __heap_end = .; } > RAM
  __stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
