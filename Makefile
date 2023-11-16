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
no_head_obj_files := $(subst .c,.o,$(c_files)) $(subst .S,.o,$(asm_files))
obj_files := $(addprefix $(obj_dir_head)/,$(no_head_obj_files))
obj_dirs := $(addprefix $(obj_dir_head)/,$(src_dirs))

# user space files
user_include_dir_head := user_space

user_src_dir_head := user_space

user_c_files := $(shell find $(user_src_dir_head)/ -name "*.c")
user_c_dirs := $(sort $(dir $(user_c_files)))
user_asm_files := $(shell find $(user_src_dir_head)/ -name "*.S")
user_asm_dirs := $(sort $(dir $(user_asm_files)))

user_obj_files := $(subst .c,.o,$(user_c_files)) $(subst .S,.o,$(user_asm_files))
user_dirs := $(sort $(user_c_dirs) $(user_asm_dirs))

user_link_files := $(obj_dir_head)/$(user_src_dir_head)/sys_call.o
user_exec_files := $(addprefix $(obj_dir_head)/,$(subst .c,,$(user_c_files)))
user_output_files := $(addprefix $(obj_dir_head)/,$(user_obj_files))
user_output_dirs := $(addprefix $(obj_dir_head)/,$(user_dirs))

# .S files to be objdump
dump_asm_files := $(subst .o,.S,$(obj_files))

# the palce of the final output kernel
the_kernel := $(obj_dir_head)/kernel
the_kernel_asm := $(addsuffix .S,$(the_kernel))

# qemu
qemu = qemu-system-riscv64

gdbport = 26000
# $(shell expr `id -u` % 5000 + 25000)
qemu_gdbopts = -S -gdb tcp::$(gdbport)

qemu_mem_size = 128M

qemu_machine_opts = -machine virt -bios none
qemu_machine_opts += -m $(qemu_mem_size) -smp $(cpus) -nographic
qemu_kernel_place_opts = -kernel $(the_kernel)
qemu_opts = $(qemu_machine_opts) $(qemu_kernel_place_opts)

# qemu-dtb
qemu_dtb_dir := $(obj_dir_head)/dtb
qemu_dtb := $(qemu_dtb_dir)/riscv64-virt.dtb
qemu_dts := $(subst .dtb,.dts,$(qemu_dtb))
qemu-dtb-opts = -machine dumpdtb=$(qemu_dtb)

# create all output dirs in out/
dirs_to_be_created := $(obj_dirs) $(user_output_dirs) $(qemu_dtb_dir)
create_all_dirs_cmd = $(shell for d in $(dirs_to_be_created); do if [ ! -d $$d ]; then mkdir -p $$d; fi; done)

kernel_and_user: $(the_kernel) user_space

# we don't import all the dependence relation, clean before make kernel
$(the_kernel): clean create_all_out_dirs $(ldscrip) $(obj_files)
	$(LD) $(LDFLAGS) -T $(ldscrip) -o $(the_kernel) $(obj_files)
	$(OBJDUMP) -S $(the_kernel) > $(the_kernel_asm)
	@echo ""

user_space: $(user_output_files) $(user_exec_files)

q: kernel_and_user
	$(qemu) $(qemu_opts)

db: CDBFLAGS = $(DB_DEEPTH)
db: clean kernel_and_user
	@echo ">>> qemu-gdb starts, run gdb-multiarch in other window"
	$(qemu) $(qemu_opts) $(qemu_gdbopts)

all_asm: $(obj_files) $(dump_asm_files)

dtb: create_all_out_dirs
	$(qemu) $(qemu_machine_opts) $(qemu-dtb-opts)
	dtc -I dtb -O dts -o $(qemu_dts) $(qemu_dtb)

# generate src .o
$(obj_dir_head)/$(src)/%.o: $(src)/%.c
	$(CC) $(CFLAGS) $(CDBFLAGS) $< -c -o $@

$(obj_dir_head)/$(src)/%.o: $(src)/%.S
	$(AS) $< -o $@

# generate user_space .o and executable
$(obj_dir_head)/$(user_src_dir_head)/%.o: $(user_src_dir_head)/%.c
	$(CC) $(CFLAGS) -I$(user_include_dir_head) $(CDBFLAGS) $< -c -o $@

$(obj_dir_head)/$(user_src_dir_head)/%.o: $(user_src_dir_head)/%.S
	$(AS) $< -o $@

$(obj_dir_head)/$(user_src_dir_head)/%: $(obj_dir_head)/$(user_src_dir_head)/%.o
	$(LD) $(LDFLAGS) -T $(user_src_dir_head)/user.ld -o $@ $< $(user_link_files)
	$(OBJDUMP) -S $@ > $@.S

# objdump
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
	@echo "user_exec_files: $(user_exec_files)"
