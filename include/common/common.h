/* common.h: Some important pointers, for compatibility */

#ifndef MEM_BASE
#define MEM_BASE	0x00800000
#endif

#define OS_SPECIFICS	((OsSpecifics*)(MEM_BASE + 0x1500))
#define OS_FIRMWARE	*(volatile unsigned int*)(MEM_BASE + 0x1400 + 0x04)
