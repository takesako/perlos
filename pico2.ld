ENTRY(reset)
MEMORY {
  FLASH (rx) : org = 0x10000000, len = 4M
  RAM (rwx)  : org = 0x20000000, len = 520K
}
SECTIONS {
  .text : { KEEP(*(.vectors)) KEEP(*(.picobin_block)) *(.text* .rodata*) } > FLASH
  .data : { __data_start = .; *(.data*) . = ALIGN(4); __data_end = .; } > RAM AT> FLASH
  __data_load = LOADADDR(.data);
  .bss (NOLOAD) : { __bss_start = .; *(.bss* COMMON) . = ALIGN(4); __bss_end = .; } > RAM
  .heap (NOLOAD) : ALIGN(8) { __heap_start = .; . += 464K; __heap_end = .; } > RAM
  __stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
