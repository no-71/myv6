#ifndef VM_SYSTEM_H_
#define VM_SYSTEM_H_

#define PGSIZE_BITS 12
#define PGSIZE (1 << PGSIZE_BITS)
#define PGSIZE_MASK ((1L << PGSIZE_BITS) - 1L)

#define MAX_LEVEL 2

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)
#define PTE_G (1 << 5)
#define PTE_A (1 << 6)
#define PTE_D (1 << 7)
#define PTE_RSW_0 (1 << 8)
#define PTE_RSW_1 (1 << 9)

#define PTE_ATTRIBUTE_BITS 10
#define PTE_ATTRIBUTE_MASK ((1L << PTE_ATTRIBUTE_BITS) - 1)
#define PTE_PPN_BITS 44
#define PTE_PPN_MASK (((1L << PTE_PPN_BITS) - 1) << PTE_ATTRIBUTE_BITS)
#define PTE_MASK (PTE_ATTRIBUTE_MASK | PTE_PPN_MASK)

#define VPN_LEVEL_N_BITS 9
#define VPN_LEVEL_N_MASK ((1L << VPN_LEVEL_N_BITS) - 1)
#define VPN_LEVEL_N(VA, LEVEL)                                                 \
    (((VA) >> (PGSIZE_BITS + (LEVEL)*VPN_LEVEL_N_BITS)) & VPN_LEVEL_N_MASK)

#define PTE_GET_PPN(PTE) (((PTE)&PTE_PPN_MASK) >> PTE_ATTRIBUTE_BITS)
#define PTE_GET_ATTRIBUTE(PTE) ((PTE)&PTE_ATTRIBUTE_MASK)
#define PTE_GET_PA(PTE) (PTE_GET_PPN(PTE) << PGSIZE_BITS)

#define MAKE_PTE(PA, ATTRIBUTE)                                                \
    ((((PA) << PTE_ATTRIBUTE_BITS) & PTE_PPN_MASK) |                           \
     ((ATTRIBUTE)&PTE_ATTRIBUTE_MASK))

static inline void sfence_vma_all()
{
    // flush all TLB entries.
    asm volatile("sfence.vma zero, zero");
}

#endif
