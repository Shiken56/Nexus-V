.section .init
.global _start

_start:
    /* Application uses upper 4KB of RAM. Stack at top of 8KB. */
    li sp, 0x2000
    call main
inf_loop:
    j inf_loop
