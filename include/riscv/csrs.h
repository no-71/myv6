#ifndef RISCV_H_
#define RISCV_H_

#include "config/basic_types.h"

#define READ_REG(REG) (*((volatile uint64 *)(REG)))
#define WRITE_REG(REG, VAL) (READ_REG(REG) = VAL)

// bits in xstatus(x=m,s,u)
#define XSTATUS_UIE (1 << 0)
#define XSTATUS_SIE (1 << 1)
#define XSTATUS_MIE (1 << 3)

#define XSTATUS_UPIE (1 << 4)
#define XSTATUS_SPIE (1 << 5)
#define XSTATUS_MPIE (1 << 7)

#define XSTATUS_SPP (1 << 8)
#define XSTATUS_MPP_0 (1 << 11)
#define XSTATUS_MPP_1 (1 << 12)

#define XSTATUS_MPRV (1 << 17)
#define XSTATUS_SUM (1 << 18)
#define XSTATUS_MXR (1 << 19)
#define XSTATUS_TVM (1 << 20)
#define XSTATUS_TW (1 << 21)
#define XSTATUS_TSR (1 << 22)

// bits in xie(x=m,s,u)
#define XIE_USIE (1 << 0)
#define XIE_SSIE (1 << 1)
#define XIE_MSIE (1 << 3)

#define XIE_UTIE (1 << 4)
#define XIE_STIE (1 << 5)
#define XIE_MTIE (1 << 7)

#define XIE_UEIE (1 << 8)
#define XIE_SEIE (1 << 9)
#define XIE_MEIE (1 << 11)

// bits in xip(x=m,s,u)
#define XIP_USIP (1 << 0)
#define XIP_SSIP (1 << 1)
#define XIP_MSIP (1 << 3)

#define XIP_UTIP (1 << 4)
#define XIP_STIP (1 << 5)
#define XIP_MTIP (1 << 7)

#define XIP_UEIP (1 << 8)
#define XIP_SEIP (1 << 9)
#define XIP_MEIP (1 << 11)

#define READ_REG_FN(REG)                                                       \
    inline uint64 r_##REG()                                                    \
    {                                                                          \
        uint64 x;                                                              \
        asm volatile("mv "                                                     \
                     "%0, " #REG                                               \
                     : "=r"(x));                                               \
        return x;                                                              \
    }

#define WRITE_REG_FN(REG)                                                      \
    inline void w_##REG(uint64 x)                                              \
    {                                                                          \
        asm volatile("csrw " #REG ", %0" ::"r"(x));                            \
    }

#define READ_CSR_FN(CSR)                                                       \
    inline uint64 r_##CSR()                                                    \
    {                                                                          \
        uint64 x;                                                              \
        asm volatile("csrr "                                                   \
                     "%0, " #CSR                                               \
                     : "=r"(x));                                               \
        return x;                                                              \
    }

#define WRITE_CSR_FN(CSR)                                                      \
    inline void w_##CSR(uint64 x)                                              \
    {                                                                          \
        asm volatile("csrw " #CSR ", %0" ::"r"(x));                            \
    }

#define READ_MS_CSR_FN(CSR) READ_CSR_FN(m##CSR) READ_CSR_FN(s##CSR)
#define WRITE_MS_CSR_FN(CSR) WRITE_CSR_FN(m##CSR) WRITE_CSR_FN(s##CSR)
#define READ_N_WRITE_MS_CSR_FN(CSR) READ_MS_CSR_FN(CSR) WRITE_MS_CSR_FN(CSR)

// trap setup
READ_N_WRITE_MS_CSR_FN(status)
READ_N_WRITE_MS_CSR_FN(ie)
READ_N_WRITE_MS_CSR_FN(tvec)

// trap handling
READ_N_WRITE_MS_CSR_FN(scratch)
READ_N_WRITE_MS_CSR_FN(epc)
READ_N_WRITE_MS_CSR_FN(cause)
READ_N_WRITE_MS_CSR_FN(tval)
READ_N_WRITE_MS_CSR_FN(ip)

// m-mode csrs
READ_CSR_FN(mhartid)
WRITE_CSR_FN(medeleg)
WRITE_CSR_FN(mideleg)

// s-mode csrs
WRITE_CSR_FN(satp)

// generic regs
READ_REG_FN(tp)

#endif
