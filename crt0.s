/* A small crt0. */
.extern main

.globl _start
_start:
	/* memset the bss */
	#lis r3,     __bss_start@h
	#ori r3, r3, __bss_start@l
	#li r4, 0
	#lis r5,     __bss_end@h
	#ori r5, r5, __bss_end@l
	#subf r5, r5, 3
	#bl memset

	b main
