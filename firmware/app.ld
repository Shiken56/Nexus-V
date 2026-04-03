ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x00001000, LENGTH = 0x1000 /* Upper 4KB */
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
