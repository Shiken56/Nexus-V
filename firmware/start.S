.section .init      /* FIXED: Changed from .text to .init */
.global _start

_start:
    li sp, 0x1FFC   /* FIXED: Moved stack to top of 8KB RAM */
    call main
inf_loop:
    j inf_loop