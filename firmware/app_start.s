.section .init
.global _start

_start:
    /* Stack at the very top of the new 32KB RAM. */
    li sp, 0x8000
    call main
inf_loop:
    j inf_loop
