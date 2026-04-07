ENTRY(_start)

MEMORY {
    /* Origin stays at 0x1000 (after bootloader). 
       Length is now Total (32KB) - Bootloader (4KB) = 28KB (0x7000) */
    RAM (rwx) : ORIGIN = 0x00001000, LENGTH = 0x7000 
}
SECTIONS {
    .text : {
        *(.init)
        *(.text)
        *(.rodata*)
    } > RAM
    
    .data : {*(.data) *(.sdata)} > RAM
    .bss  : {*(.bss) *(.sbss)} > RAM
}
