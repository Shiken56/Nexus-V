.section .text
.global _start

_start:
    li sp, 0x1000
    call main
inf_loop:
    j inf_loop

    