/* os_defs.h: The _OsSpecifics structure, as needed by dynamic_libs */

typedef struct _OsSpecifics
{
	unsigned int addr_OSDynLoad_Acquire;
	unsigned int addr_OSDynLoad_FindExport;
	unsigned int addr_OSTitle_main_entry;

	unsigned int addr_KernSyscallTbl1;
	unsigned int addr_KernSyscallTbl2;
	unsigned int addr_KernSyscallTbl3;
	unsigned int addr_KernSyscallTbl4;
	unsigned int addr_KernSyscallTbl5;
} OsSpecifics;
