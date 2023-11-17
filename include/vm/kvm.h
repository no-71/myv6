#ifndef KVM_H_
#define KVM_H_

#include "vm/vm.h"
void kvm_init(void);
void kvm_init_hart(void);

extern page_table kernel_page_table;

#endif
