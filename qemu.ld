ENTRY(reset)
MEMORY {
 CODE (rx) : ORIGIN = 0x10000000, LENGTH = 4M
 RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 16M
}
SECTIONS {
 .vectors : { KEEP(*(.vectors)) } > CODE
 .text : { *(.text*) *(.rodata*) } > CODE
 .data : { __data_start = .; *(.data*) . = ALIGN(4); __data_end = .; } > RAM
 __data_load = LOADADDR(.data);
 .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) . = ALIGN(4); __bss_end = .; } > RAM
 __stack_top = ORIGIN(RAM) + LENGTH(RAM);
}