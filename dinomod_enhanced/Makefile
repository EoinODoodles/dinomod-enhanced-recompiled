BUILD_DIR := build

CC      := clang
LD      := ld.lld
TARGET  := $(BUILD_DIR)/mod.elf

LDSCRIPT := ../mod.ld
CFLAGS   := -target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls -mno-odd-spreg -mno-check-zero-division \
			-fomit-frame-pointer -ffast-math -fno-unsafe-math-optimizations -fno-builtin \
			-Wall -Wextra -Wno-incompatible-library-redeclaration -Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-variable \
			-Wno-missing-braces -Wno-unsupported-floating-point-opt -Werror=section
CPPFLAGS := -nostdinc -D_LANGUAGE_C -DMIPS -D_MIPS_SZLONG=32 -DF3DEX_GBI_2 \
			-I ../lib/dino-recomp-mod-api/include -I ../lib/dino-recomp-decomp-bridge/include -I ../lib/dino-recomp-decomp-bridge/dinosaur-planet/include \
		    -I ../lib/extfs/include -I include
LDFLAGS  := -nostdlib -T $(LDSCRIPT) -Map $(BUILD_DIR)/mod.map --unresolved-symbols=ignore-all --emit-relocs -e 0 --no-nmagic

C_SRCS := $(wildcard src/*.c)
C_SRCS += $(wildcard src/core/*.c)
C_SRCS += $(wildcard src/custom/*.c)
C_SRCS += $(wildcard src/dlls/engine/*.c)
C_SRCS += $(wildcard src/dlls/objects/*.c)
C_OBJS := $(addprefix $(BUILD_DIR)/, $(C_SRCS:.c=.o))
C_DEPS := $(addprefix $(BUILD_DIR)/, $(C_SRCS:.c=.d))

BUILD_DIRS := $(BUILD_DIR) $(BUILD_DIR)/src $(BUILD_DIR)/src/core $(BUILD_DIR)/src/custom \
	          $(BUILD_DIR)/src/dlls/engine $(BUILD_DIR)/src/dlls/objects

$(TARGET): $(C_OBJS) $(LDSCRIPT)
	$(LD) $(C_OBJS) $(LDFLAGS) -o $@

$(BUILD_DIRS):
ifeq ($(OS),Windows_NT)
	mkdir $(subst /,\,$@)
else
	mkdir -p $@
endif

$(C_OBJS): $(BUILD_DIR)/%.o : %.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -MMD -MF $(@:.o=.d) -c -o $@

clean:
	rm -rf $(BUILD_DIR)

-include $(C_DEPS)

.PHONY: clean
