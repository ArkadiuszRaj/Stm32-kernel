# This makefile accepts two params: KERNEL_ARCH and KERNEL_COMPILER. Both are mandatory.
#
#  KERNEL_ARCH: the following values are valid:
#
#     cortex_m0
#     cortex_m0plus
#     cortex_m1
#     cortex_m3
#     cortex_m4
#     cortex_m4f
#
#  KERNEL_COMPILER: depends on KERNEL_ARCH.
#     For cortex-m series, the following values are valid:
#
#        arm-none-eabi-gcc
#        clang
#
#  Example invocation:
#
#     $ make KERNEL_ARCH=cortex_m3 KERNEL_COMPILER=arm-none-eabi-gcc
#

CFLAGS_COMMON = -Wall -Wunused-parameter -Werror -ffunction-sections -fdata-sections -g3 -Os


#---------------------------------------------------------------------------
# Cortex-M series
#---------------------------------------------------------------------------

ifeq ($(KERNEL_ARCH), $(filter $(KERNEL_ARCH), cortex_m0 cortex_m0plus cortex_m1 cortex_m3 cortex_m4 cortex_m4f))
   KERNEL_ARCH_DIR = cortex_m

   ifeq ($(KERNEL_COMPILER), $(filter $(KERNEL_COMPILER), arm-none-eabi-gcc clang))

      ifeq ($(KERNEL_ARCH), cortex_m0)
         CORTEX_M_FLAGS = -mcpu=cortex-m0 -mfloat-abi=soft
      endif
      ifeq ($(KERNEL_ARCH), cortex_m0plus)
         CORTEX_M_FLAGS = -mcpu=cortex-m0plus -mfloat-abi=soft
      endif
      ifeq ($(KERNEL_ARCH), cortex_m1)
         CORTEX_M_FLAGS = -mcpu=cortex-m1 -mfloat-abi=soft
      endif
      ifeq ($(KERNEL_ARCH), cortex_m3)
         CORTEX_M_FLAGS = -mcpu=cortex-m3 -mfloat-abi=soft
      endif
      ifeq ($(KERNEL_ARCH), cortex_m4)
         CORTEX_M_FLAGS = -mcpu=cortex-m4 -mfloat-abi=soft
      endif
      ifeq ($(KERNEL_ARCH), cortex_m4f)
         CORTEX_M_FLAGS = -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
      endif

     CC = arm-none-eabi-gcc
     AR = arm-none-eabi-ar
     CFLAGS = $(CORTEX_M_FLAGS) $(CFLAGS_COMMON) -mthumb -fsigned-char -pedantic
     ASFLAGS = $(CFLAGS) -x assembler-with-cpp
     KERNEL_COMPILER_VERSION_CMD := $(CC) --version

     BINARY_CMD = $(AR) -r $(BINARY) $(OBJS)
   endif
endif


# check if KERNEL_ARCH and KERNEL_COMPILER values were recognized

ifeq ($(KERNEL_ARCH),)
   $(error KERNEL_ARCH is undefined. $(ERR_MSG_STD))
endif

ifndef KERNEL_ARCH_DIR
   $(error KERNEL_ARCH has invalid value. $(ERR_MSG_STD))
endif

ifndef BINARY_CMD
   $(error KERNEL_COMPILER has invalid value. $(ERR_MSG_STD))
endif





SOURCE_DIR     = src
BIN_DIR        = bin/$(KERNEL_ARCH)/$(KERNEL_COMPILER)
OBJ_DIR        = _obj/$(KERNEL_ARCH)/$(KERNEL_COMPILER)

CPPFLAGS = -I${SOURCE_DIR} -I${SOURCE_DIR}/core -I${SOURCE_DIR}/core/internal -I${SOURCE_DIR}/arch

# get just all headers
HEADERS  := $(shell find ${SOURCE_DIR}/ -name "*.h")

# get all core sources plus sources for needed platform
SOURCES  := $(wildcard $(SOURCE_DIR)/core/*.c $(SOURCE_DIR)/arch/$(KERNEL_ARCH_DIR)/*.c $(SOURCE_DIR)/arch/$(KERNEL_ARCH_DIR)/*.S)

# generate list of all object files from source files
OBJS     := $(patsubst %.c,$(OBJ_DIR)/%.o,$(patsubst %.S,$(OBJ_DIR)/%.o,$(notdir $(SOURCES))))

# generate binary library file
BINARY = $(BIN_DIR)/kernel_$(KERNEL_ARCH)_$(KERNEL_COMPILER).a

# command that creates necessary directories, must be used in rules below
MKDIR_P_CMD = @mkdir -p $(@D)

README_FILE = $(BIN_DIR)/readme.txt
BUILD_LOG_FILE = $(BIN_DIR)/build_log.txt

REDIRECT_CMD = | tee $(BUILD_LOG_FILE)


# this rule is needed to redirect build log to a file
all:
	mkdir -p $(BIN_DIR)
	touch $(BUILD_LOG_FILE)
	bash -c 'set -o pipefail; $(MAKE) all-actual $(REDIRECT_CMD)'

# this is actual 'all' rule
all-actual: $(BINARY)

#-- for simplicity, every object file just depends on any header file
$(OBJS): $(HEADERS)

$(BINARY): $(OBJS)
	$(MKDIR_P_CMD)
	$(BINARY_CMD)
	@echo "" > $(README_FILE)
	@echo "\nKERNEL version:" >> $(README_FILE)
	@bash stuff/scripts/git_ver_echo.sh >> $(README_FILE)
	@echo "" >> $(README_FILE)
	@echo "\nArchitecture:" >> $(README_FILE)
	@echo "$(KERNEL_ARCH)" >> $(README_FILE)
	@echo "\nCompiler:" >> $(README_FILE)
	@echo "$(KERNEL_COMPILER)" >> $(README_FILE)
	@echo "\nCompiler version:" >> $(README_FILE)
	@$(KERNEL_COMPILER_VERSION_CMD) >> $(README_FILE)

$(OBJ_DIR)/%.o : $(SOURCE_DIR)/core/%.c
	$(MKDIR_P_CMD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o : $(SOURCE_DIR)/arch/$(KERNEL_ARCH_DIR)/%.c
	$(MKDIR_P_CMD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o : $(SOURCE_DIR)/arch/$(KERNEL_ARCH_DIR)/%.S
	$(MKDIR_P_CMD)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<


