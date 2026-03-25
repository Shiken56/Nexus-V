MEMORY {
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 0x1000
}

SECTIONS {
    .text : {*(.text)} > RAM
    .data : {*(.data) *(.sdata)} > RAM
    .bss  : {*(.bss) *(.sbss)} > RAM
}