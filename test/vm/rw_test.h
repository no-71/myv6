#ifndef RW_TEST_H_
#define RW_TEST_H_

#include "config/basic_types.h"
#include "test/test_config.h"
#include "util/kprint.h"
#include "vm/kalloc.h"
#include "vm/memory_layout.h"

extern struct mem_list mem_container;

static inline void rw_mem(struct node *cur, void (*map)(uint64 *, uint64))
{
    uint64 write_offset[] = { 0x30, 0x210, 0x340, 0x370, 0x490, 0x920 };
    int sz = sizeof(write_offset) / sizeof(uint64);
    uint64 startno = 17;
    uint64 curoff = 0;

    while (cur != NULL) {
        uint64 val = startno + curoff;
        for (int i = 0; i < sz; i++) {
            uint64 *w = (uint64 *)((char *)cur + write_offset[i]);
            map(w, val);
        }
        cur = cur->next;
        curoff++;
    }
}

static inline void write_to_mem(uint64 *w, uint64 val) { *w = val; }

static inline void check_mem(uint64 *w, uint64 val)
{
    if (*w != val) {
        PANIC_FN("find incorresponding value (compared with write value)");
    }
#define GARBAGE_VAL 0x0505##0505##0505##0505
    *w = GARBAGE_VAL;
}

static inline void rw_mem_test(void)
{
#ifdef TEST_ENABLE
    struct node *cur = mem_container.head;
    if (cur == NULL) {
        KPRINT_FN("mem head is NULL\n");
        return;
    }

    rw_mem(cur, write_to_mem);
    rw_mem(cur, check_mem);

    KPRINT_FN("succesfull\n");
#endif
}

#endif
