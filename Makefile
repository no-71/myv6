# basic config
toolprefix = riscv64-linux-gnu-
cpus = 1

# compile
CC = $(toolprefix)gcc
LD = $(toolprefix)ld
OBJDUMP = $(toolprefix)objdump

CFLAGS = -Wall -Werror -fno-omit-frame-pointer -ggdb
CFLAGS += -O
CFLAGS += -I.
CFLAGS += -fno-stack-protector
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax

LDFLAGS = -z max-page-size=4096
ldscrip = $(src)/kernel.ld

# qemu
qemu = qemu-system-risv64

qemu_opts = -machine virt -bios none -kernel $(kernel) -m 128M -smp $(cpus) -nographic

# src
include_dir_head := include

src := src
c_files := $(wildcard $(shell find $(src)/ -name "*.c"))
c_dirs := $(sort $(dir $(c_files)))
asm_files := $(wildcard $(shell find $(src)/ -name "*.S"))
asm_dirs := $(sort $(dir $(asm_files)))

src_files := $(c_files) $(asm_files)
src_dirs := $(sort $(c_dirs) $(asm_dirs))

obj_dir_head := out
obj_files_0 := $(addprefix $(obj_dir_head)/,$(subst .c,.o,$(c_files)) $(subst .S,.o,$(asm_files)))
obj_dirs := $(addprefix $(obj_dir_head)/,$(src_dirs))
# the ENTRY(_entry) in kernel.ld won't works somhow, try to fix it in this ugly ways.
# sweat like a pig, bro.
obj_files := $(filter %/entry.o,$(obj_files_0)) $(filter-out %/entry.o, $(obj_files_0))

dump_asm_files := $(subst .o,.S,$(obj_files))

the_kernel := $(obj_dir_head)/kernel
the_kernel_asm := $(addsuffix .S,$(the_kernel))

# create all output dirs in out/
create_all_obj_dirs := $(shell for d in $(obj_dirs); do if [ ! -d $$d ]; then mkdir -p $$d; fi; done)

kernel: $(obj_files)
	$(LD) $(LDFLAGS) -T $(ldscrip) -o $(the_kernel) $(obj_files)
	$(OBJDUMP) -S $(the_kernel) > $(the_kernel_asm)

all_asm: $(obj_files) $(dump_asm_files)

$(obj_dir_head)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

$(obj_dir_head)/%.o: %.S
	$(CC) $(CFLAGS) $< -c -o $@

$(obj_dir_head)/%.S: $(obj_dir_head)/%.o
	$(OBJDUMP) -S $< > $@

.PHONEY: clean
clean:
	rm -r $(obj_dir_head)/*

# debug
.PHONEY: datas
datas:
	@echo "c_files: $(c_files)"
	@echo "c_dirs: $(c_dirs)"
	@echo "asm_files: $(asm_files)"
	@echo "asm_dirs: $(asm_dirs)"
	@echo "src_files: $(src_files)"
	@echo "src_dirs: $(src_dirs)"
	@echo "obj_files: $(obj_files)"
	@echo "obj_dirs: $(obj_dirs)"
