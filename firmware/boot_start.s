.section .init
.global _start

_start:
    /* Bootloader uses first 4KB of RAM. Stack at top of 4KB. */
    li sp, 0x1000
    call main
inf_loop:
    j inf_loop
