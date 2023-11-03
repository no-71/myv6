#ifndef CLINT_H_
#define CLINT_H_

#define CLINT 0x2000000L
#define CLINT_MTIMECMP(HART_ID) (CLINT + 0x4000 + 8 * (HART_ID))
#define CLINT_MTIME (CLINT + 0xBFF8)

#endif
