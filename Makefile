# command configuration:
# cpus=n : n cpus
# alldb=1 : make the common compile use the same flag(-ggdb3) as the db compile

# basic config
toolprefix = riscv64-linux-gnu-
ifndef cpus
    cpus = 1
endif

# compile
CC = $(toolprefix)gcc
AS = $(toolprefix)as
LD = $(toolprefix)ld
OBJDUMP = $(toolprefix)objdump

CFLAGS = -Wall -Werror -fno-omit-frame-pointer
CFLAGS += -O
CFLAGS += $(addprefix -I,$(include_dir_head))
CFLAGS += -fno-stack-protector
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
DB_DEEPTH = -ggdb3
ifdef alldb
    CDBFLAGS = $(DB_DEEPTH)
endif

LDFLAGS = -z max-page-size=4096
ldscrip = $(src)/kernel.ld

# include files
include_dir_head := include .

# c and asm files in src
src := src
c_files := $(shell find $(src)/ -name "*.c")
c_dirs := $(sort $(dir $(c_files)))
asm_files := $(shell find $(src)/ -name "*.S")
asm_dirs := $(sort $(dir $(asm_files)))

# all src files
src_files := $(c_files) $(asm_files)
src_dirs := $(sort $(c_dirs) $(asm_dirs))

# output files
obj_dir_head := out
obj_files := $(addprefix $(obj_dir_head)/,$(subst .c,.o,$(c_files)) $(subst .S,.o,$(asm_files)))
obj_dirs := $(addprefix $(obj_dir_head)/,$(src_dirs))

# .S files to be objdump
dump_asm_files := $(subst .o,.S,$(obj_files))

# the palce of the final output kernel
the_kernel := $(obj_dir_head)/kernel
the_kernel_asm := $(addsuffix .S,$(the_kernel))

# qemu
qemu = qemu-system-riscv64

gdbport = $(shell expr `id -u` % 5000 + 25000)
qemu_gdbopts = -S -gdb tcp::$(gdbport)

qemu_machine_opts = -machine virt -bios none
qemu_machine_opts += -m 128M -smp $(cpus) -nographic
qemu_kernel_place_opts = -kernel $(the_kernel)
qemu_opts = $(qemu_machine_opts) $(qemu_kernel_place_opts)

# qemu-dtb
qemu_dtb_dir := $(obj_dir_head)/dtb
qemu_dtb := $(qemu_dtb_dir)/riscv64-virt.dtb
qemu_dts := $(subst .dtb,.dts,$(qemu_dtb))
qemu-dtb-opts = -machine dumpdtb=$(qemu_dtb)

# create all output dirs in out/
dirs_to_be_created := $(obj_dirs) $(qemu_dtb_dir)
create_all_dirs_cmd = $(shell for d in $(dirs_to_be_created); do if [ ! -d $$d ]; then mkdir -p $$d; fi; done)

# we don't import all the dependence relation, clean before make kernel
$(the_kernel): clean create_all_out_dirs $(ldscrip) $(obj_files)
	$(LD) $(LDFLAGS) -T $(ldscrip) -o $(the_kernel) $(obj_files)
	$(OBJDUMP) -S $(the_kernel) > $(the_kernel_asm)
	@echo ""

q: $(the_kernel)
	$(qemu) $(qemu_opts)

db: CDBFLAGS = $(DB_DEEPTH)
db: clean $(the_kernel)
	@echo ">>> qemu-gdb starts, run gdb-multiarch in other window"
	$(qemu) $(qemu_opts) $(qemu_gdbopts)

all_asm: $(obj_files) $(dump_asm_files)

dtb: create_all_out_dirs
	$(qemu) $(qemu_machine_opts) $(qemu-dtb-opts)
	dtc -I dtb -O dts -o $(qemu_dts) $(qemu_dtb)

$(obj_dir_head)/%.o: %.c
	$(CC) $(CFLAGS) $(CDBFLAGS) $< -c -o $@

$(obj_dir_head)/%.o: %.S
	$(AS) $< -o $@

$(obj_dir_head)/%.S: $(obj_dir_head)/%.o
	$(OBJDUMP) -S $< > $@

.PHONEY: create_all_out_dirs
create_all_out_dirs:
	$(create_all_dirs_cmd)

.PHONEY: clean
clean:
	rm -rf $(obj_dir_head)/*

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
