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


.globl __eabi
# According to the GCC manpage, "Selecting -meabi means that the stack is
# aligned to an 8 byte boundary, a function "__eabi" is called to from "main"
# to set up the eabi environment, and the -msdata option can use both "r2" and
# "r13" to point to two separate small data areas. [...] The -meabi option is
# on by default if you configured GCC using one of the powerpc*-*-eabi*
# options."
#
# Since devkitPPC is such a powerpc-eabi toolchain, we have to implement __eabi.
__eabi:
	lis r13,      __sdata_start@h
	ori r13, r13, __sdata_start@l
	lis r2,     __sdata2_start@h
	ori r2, r2, __sdata2_start@l
	blr
