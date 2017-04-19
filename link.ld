/* Linker script, mostly copied from the usual link.ld */

SECTIONS {
	/* FIXME: This base address doesn't work because ld thinks the
	 * resulting segment has an alignment of 0x10000 (64k), and puts it at
	 * 0x800000. */
	/*. = 0x00802000;*/
	. = 0x00810000;

	.text : {
		*(.text*);
	}
	.rodata : {
		*(.rodata*);
	}
	.data : {
		*(.data*);

		__sdata_start = .;
		*(.sdata*);
		__sdata_end = .;

		__sdata2_start = .;
		*(.sdata2*);
		__sdata2_end = .;
	}
	.bss : {
		__bss_start = .;
		*(.bss*);
		*(.sbss*);
		*(COMMON);
		__bss_end = .;
	}
	__CODE_END = .;

	/DISCARD/ : {
		*(*);
	}
}
