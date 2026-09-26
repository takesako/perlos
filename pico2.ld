ENTRY(reset)
MEMORY {
 FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 4M
 RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 520K
}
SECTIONS {
 .text : { KEEP(*(.vectors)) KEEP(*(.picobin_block)) *(.text*) *(.rodata*) } > FLASH
 .data : { __data_start = .; *(.data*) . = ALIGN(4); __data_end = .; } > RAM AT> FLASH
 __data_load = LOADADDR(.data);
 .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) . = ALIGN(4); __bss_end = .; } > RAM
 .heap (NOLOAD) : ALIGN(8) { __heap_start = .; . += 393216; __heap_end = .; } > RAM
 __stack_top = ORIGIN(RAM) + LENGTH(RAM);
}