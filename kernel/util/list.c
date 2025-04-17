#include "util/kprint.h"
#include "util/list_include.h"

#ifdef CONFIG_DEBUG_LIST
#define __list_valid_slowpath
#else
#define __list_valid_slowpath __cold __preserve_most
#endif

bool __list_valid_slowpath __list_add_valid_or_report(struct list_head *new,
                                                      struct list_head *prev,
                                                      struct list_head *next)
{
    panic("__list_add_valid_or_report");
}

bool __list_valid_slowpath
__list_del_entry_valid_or_report(struct list_head *entry)
{
    panic("__list_del_entry_valid_or_report");
}
