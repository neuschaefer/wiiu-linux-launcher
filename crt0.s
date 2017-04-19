/* A small crt0. */
.extern main
.globl _start

_start:
	/* memset the bss */
	#lis 3, __bss_start@h
	#ori 3, 3, __bss_start@l
	#li 4, 0
	#lis 5, __bss_end@h
	#ori 5, 5, __bss_end@l
	#subf 5, 5, 3
	#bl memset

	b main
