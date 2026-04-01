ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 0x2000 /* FIXED: Expanded to 8KB */
}

SECTIONS {
    .text : {
        *(.init)       /* FIXED: Bootloader is forced to exactly 0x0000_0000 */
        *(.text)
        *(.rodata*)    /* FIXED: Strings now live in RAM */
    } > RAM
    
    .data : {*(.data) *(.sdata)} > RAM
    .bss  : {*(.bss) *(.sbss)} > RAM
}