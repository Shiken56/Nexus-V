ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 0x1000 /* Lower 4KB */
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
