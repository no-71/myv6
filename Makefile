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

# src dir head
src := src

# include files
include_dir_head := include .

# output dir head
output_dir_head := out

# c and asm files in src
c_files := $(shell find $(src)/ -name "*.c")
c_dirs := $(sort $(dir $(c_files)))
asm_files := $(shell find $(src)/ -name "*.S")
asm_dirs := $(sort $(dir $(asm_files)))

# all src files
src_files := $(c_files) $(asm_files)
src_dirs := $(sort $(c_dirs) $(asm_dirs))

# src output files
src_obj_files := $(subst .c,.o,$(c_files)) $(subst .S,.o,$(asm_files))
src_output_files := $(addprefix $(output_dir_head)/,$(src_obj_files))
src_output_dirs := $(addprefix $(output_dir_head)/,$(src_dirs))

# user space files
user_include_dir_head := user_space

user_src_dir_head := user_space

user_c_files := $(shell find $(user_src_dir_head)/ -name "*.c")
user_c_dirs := $(sort $(dir $(user_c_files)))
user_asm_files := $(shell find $(user_src_dir_head)/ -name "*.S")
user_asm_dirs := $(sort $(dir $(user_asm_files)))

user_obj_files := $(subst .c,.o,$(user_c_files)) $(subst .S,.o,$(user_asm_files))
user_dirs := $(sort $(user_c_dirs) $(user_asm_dirs))

user_link_files := $(output_dir_head)/$(user_src_dir_head)/sys_call.o
user_exec_files := $(addprefix $(output_dir_head)/,$(subst .c,,$(user_c_files)))
user_output_files := $(addprefix $(output_dir_head)/,$(user_obj_files))
user_output_dirs := $(addprefix $(output_dir_head)/,$(user_dirs))

# elf to binary
elf_to_binary := test/fake_fs/e2b
binary_var_suffix := _binary
binary_file_suffix := _binary.c
binary_dir := $(output_dir_head)/binary_code

user_output_binary_files := $(addprefix $(binary_dir)/,$(addsuffix $(binary_file_suffix),$(notdir $(user_exec_files))))
user_output_code_obj_files := $(subst .c,.o,$(user_output_binary_files))

# .S files to be objdump
dump_asm_files := $(subst .o,.S,$(src_output_files))

# the palce of the final output kernel
the_kernel := $(output_dir_head)/kernel
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
qemu_dtb_dir := $(output_dir_head)/dtb
qemu_dtb := $(qemu_dtb_dir)/riscv64-virt.dtb
qemu_dts := $(subst .dtb,.dts,$(qemu_dtb))
qemu-dtb-opts = -machine dumpdtb=$(qemu_dtb)

# create all output dirs in out/
dirs_to_be_created := $(src_output_dirs) $(user_output_dirs) $(binary_dir) $(qemu_dtb_dir)
create_all_dirs_cmd = $(shell for d in $(dirs_to_be_created); do if [ ! -d $$d ]; then mkdir -p $$d; fi; done)

# we don't import all the dependence relation, clean before make kernel
kernel_and_user: clean create_all_out_dirs user_space $(the_kernel)

$(the_kernel): $(ldscrip) $(src_output_files) $(user_output_code_obj_files)
	$(LD) $(LDFLAGS) -T $(ldscrip) -o $(the_kernel) $(src_output_files) $(user_output_code_obj_files)
	$(OBJDUMP) -S $(the_kernel) > $(the_kernel_asm)
	@echo ""

user_space: $(user_output_files) $(user_exec_files)

q: kernel_and_user
	$(qemu) $(qemu_opts)

db: CDBFLAGS = $(DB_DEEPTH)
db: clean kernel_and_user
	@echo ">>> qemu-gdb starts, run gdb-multiarch in other window"
	$(qemu) $(qemu_opts) $(qemu_gdbopts)

all_asm: $(src_output_files) $(dump_asm_files)

dtb: create_all_out_dirs
	$(qemu) $(qemu_machine_opts) $(qemu-dtb-opts)
	dtc -I dtb -O dts -o $(qemu_dts) $(qemu_dtb)

# generate src .o
$(output_dir_head)/$(src)/%.o: $(src)/%.c
	$(CC) $(CFLAGS) $(CDBFLAGS) $< -c -o $@

$(output_dir_head)/$(src)/%.o: $(src)/%.S
	$(AS) $< -o $@

# for binary var files, don't know why more accurate path won't work
out/%.o: out/%.c
	$(CC) $(CFLAGS) $(CDBFLAGS) $< -c -o $@

# generate user_space .o and executable
$(output_dir_head)/$(user_src_dir_head)/%.o: $(user_src_dir_head)/%.c
	$(CC) $(CFLAGS) -I$(user_include_dir_head) $(CDBFLAGS) $< -c -o $@

$(output_dir_head)/$(user_src_dir_head)/%.o: $(user_src_dir_head)/%.S
	$(AS) $< -o $@

$(output_dir_head)/$(user_src_dir_head)/%: $(output_dir_head)/$(user_src_dir_head)/%.o
	$(LD) $(LDFLAGS) -T $(user_src_dir_head)/user.ld -o $@ $< $(user_link_files)
	$(OBJDUMP) -S $@ > $@.S
	./$(elf_to_binary) $(notdir $@)$(binary_var_suffix) < $@ > $(binary_dir)/$(notdir $@)$(binary_file_suffix)

# objdump
$(output_dir_head)/$(src)/%.S: $(output_dir_head)/$(src)/%.o
	$(OBJDUMP) -S $< > $@

.PHONEY: create_all_out_dirs
create_all_out_dirs:
	$(create_all_dirs_cmd)

.PHONEY: clean
clean:
	rm -rf $(output_dir_head)/*

# debug
.PHONEY: datas
datas:
	@echo "c_files: $(c_files)"
	@echo "c_dirs: $(c_dirs)"
	@echo "asm_files: $(asm_files)"
	@echo "asm_dirs: $(asm_dirs)"
	@echo "src_files: $(src_files)"
	@echo "src_dirs: $(src_dirs)"
	@echo "src_output_files: $(src_output_files)"
	@echo "src_output_dirs: $(src_output_dirs)"
	@echo "user_exec_files: $(user_exec_files)"
	@echo "user_output_code_obj_files: $(user_output_code_obj_files)"
	@echo "binary_dir:" $(binary_dir)
