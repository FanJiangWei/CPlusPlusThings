# Makefile, do not edit the "empty" all target, will be the first target
include Rules.make
include config/Macro.define

CONFIG_XTENSA_LINUX_GCC := y
CONFIG_RISCV_LINUX_GCC  := y

###########################################################
#
#	define xtensa variable
#
###########################################################
CONFIG_INSTALL_PATH="/opt"
CONFIG_XTENSA_CORE="DC_D_233L"

ifeq ($(call qstrip, $(CONFIG_XTENSA_LINUX_GCC)), y)
	CONFIG_XTENSA_VER="."
	CONFIG_XTENSA_PATH="xtensa_toolchain/output/host/usr"
else
	CONFIG_XTENSA_FLAG="RD-2012.5-linux"
	CONFIG_XTENSA_VER="RD-2012.5-linux/XtensaTools"
	CONFIG_XTENSA_PATH="xtensa-4.0.5/XtDevTools/install/tools"
endif

XTENSA_BIN_DIR := $(call qstrip,$(CONFIG_INSTALL_PATH))/$(call qstrip,$(CONFIG_XTENSA_PATH))/$(call qstrip,$(CONFIG_XTENSA_VER))/bin
XTENSA_FLAG := --xtensa-system=$(call qstrip,$(CONFIG_INSTALL_PATH))/$(call qstrip,$(CONFIG_XTENSA_PATH))/../builds/$(call qstrip,$(CONFIG_XTENSA_FLAG))/$(call qstrip,$(CONFIG_XTENSA_CORE))/config --xtensa-core=$(call qstrip,$(CONFIG_XTENSA_CORE)) --xtensa-params=
RISCV_BIN_DIR := $(call qstrip,$(CONFIG_INSTALL_PATH))/riscv64-elf-x86_64/bin

###########################################################
#
#	define system variable
#
###########################################################

ifeq ($(call qstrip, $(CONFIG_XTENSA_LINUX_GCC)), y)
	CC := $(XTENSA_BIN_DIR)/xtensa-linux-gcc
	AR := $(XTENSA_BIN_DIR)/xtensa-linux-ar
	OBJCOPY := $(XTENSA_BIN_DIR)/xtensa-linux-objcopy
	RUNLIB := $(XTENSA_BIN_DIR)/xtensa-linux-ranlib
	NEWLIB_PATH = $(CONFIG_INSTALL_PATH)/xtensa_toolchain/xtensa-linux
	MACROS := -DD233L -DXTENSA_LINUX_GCC -DROUND_ROBIN -I $(NEWLIB_PATH)/include -I $(NEWLIB_PATH)
	MKIMGFLAGS := -c -a $(call qstrip,$(CONFIG_XTENSA_CORE))
else
	CC := $(XTENSA_BIN_DIR)/xt-xcc
	AR := $(XTENSA_BIN_DIR)/xt-ar
	OBJCOPY := $(XTENSA_BIN_DIR)/xt-objcopy
	RUNLIB := $(XTENSA_BIN_DIR)/xt-ranlib
	MACROS := $(XTENSA_FLAG) -DROUND_ROBIN
endif

ifeq ($(call qstrip, $(CONFIG_RISCV_LINUX_GCC)), y)
	CC2 := $(RISCV_BIN_DIR)/riscv64-unknown-elf-gcc
	AR2 := $(RISCV_BIN_DIR)/riscv64-unknown-elf-ar
	OBJCOPY2 := $(RISCV_BIN_DIR)/riscv64-unknown-elf-objdump
	MACROS2  := -DRISCV -mabi=ilp32d -march=rv32imafdcxthead -DROUND_ROBIN
	MKIMGFLAGS2 := -c -a RISCV
endif

MKIMG := $(PRJ_SCRIPT_DIR)/mkimg
ARFLAGS := r
COPYFLAGS := -O binary $(XTENSA_FLAG)

MACROS  += -DISR_NEST_ALLOW -DDISPLAY_MMSG_DETAIL
MACROS2 += -DISR_NEST_ALLOW -DDISPLAY_MMSG_DETAIL

ifdef PROVINCE
    MACROS += -D$(PROVINCE)
    MACROS2 += -D$(PROVINCE)
else
    ifdef P
        PROVINCE = $(P)
    else
        PROVINCE = GW
    endif
    MACROS += -D$(PROVINCE)
    MACROS2 += -D$(PROVINCE)
endif

PRJ_OBJ_DIR  := ./bin
PRJ_OBJ_DIR2 := ./bin

# List of target goals
UNICORN_TARGETS := unicorn2m_sta unicorn2m_clt2 unicorn8m_cco
ROLAND_TARGETS := roland1_1m_sta roland1_1m_clt2 roland9_1m_cco

# Default CHIP_MACRO value (fallback)
CHIP_MACRO := -DCHIP_D

# Check if MAKECMDGOALS contains a target from UNICORN_TARGETS
ifneq ($(filter $(UNICORN_TARGETS), $(MAKECMDGOALS)),)
    CHIP_MACRO := -DCHIP_U
# Check if MAKECMDGOALS contains a target from ROLAND_TARGETS
else ifneq ($(filter $(ROLAND_TARGETS), $(MAKECMDGOALS)),)
    CHIP_MACRO := -DCHIP_R
endif

# Export the determined CHIP_MACRO value
export CHIP_MACRO

# Add CHIP_MACRO to MACROS and MACROS2
MACROS += $(CHIP_MACRO)
MACROS2 += $(CHIP_MACRO)

ifneq ($(findstring $(MAKECMDGOALS), unicorn2m_sta unicorn2m_clt2 unicorn8m_cco roland1_1m_sta roland1_1m_clt2 roland9_1m_cco), )
	LSP := $(PRJ_LSP_DIR)/$(MAKECMDGOALS)
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_2016
	MACROS += -DASIC_VERSION -D$(shell echo $(MAKECMDGOALS) | tr a-z A-Z)
endif

ifeq ($(MAKECMDGOALS),unicorn2m_sta)
	LSP := $(PRJ_LSP_DIR)/unicorn2m
	MACROS += -DUNICORN2M -DTRIDUCTOR -DZC3750STA -DSTA_BOARD_2_0_01 -DSTA_BOARD_2_1_00 -DSTA_BOARD_3_0_02
	MACROS += -DDDR_MEM_SIZE=2
	PRJ_OBJ_DIR := ./bin/unicorn2m_sta
	MACROS += -DSTATIC_NODE -DSTD_DUAL
	CONFIG_MIRACL = y
else ifeq ($(MAKECMDGOALS),unicorn2m_clt2)
	LSP := $(PRJ_LSP_DIR)/unicorn2m
	MACROS += -DUNICORN2M -DTRIDUCTOR -DZC3750CJQ2 -DSTA_BOARD_2_0_01
	MACROS += -DDDR_MEM_SIZE=2
	PRJ_OBJ_DIR := ./bin/unicorn2m_clt2
	MACROS += -DSTATIC_NODE -DSTD_DUAL 
	CONFIG_MIRACL = y
else ifeq ($(MAKECMDGOALS),unicorn8m_cco)
	LSP := $(PRJ_LSP_DIR)/unicorn8m
	MACROS += -DASIC_VERSION -DUNICORN8M -DTRIDUCTOR -DZC3951CCO -DCCO_BOARD_2_0_01
	MACROS += -DDDR_MEM_SIZE=8
	PRJ_OBJ_DIR := ./bin/unicorn8m_cco
	MACROS += -DSTATIC_MASTER -DSTD_DUAL 
	CONFIG_MIRACL = y
	CONFIG_GE_DRV=y
else ifeq ($(MAKECMDGOALS),roland1_1m_sta)
	LSP := $(PRJ_LSP_DIR)/roland1_1m
	MACROS +=  -DROLAND1_1M -DTRIDUCTOR -DZC3750STA -DSTA_BOARD_2_0_01 -DSTA_BOARD_2_1_00 -DSTA_BOARD_3_0_02 -DSTA_BOARD_5_0_02
	MACROS += -DDDR_MEM_SIZE=0
	PRJ_OBJ_DIR := ./bin/roland1_1m_sta
	MACROS += -DSTATIC_NODE -DSTD_DUAL 
	CONFIG_MIRACL = y
else ifeq ($(MAKECMDGOALS),roland1_1m_clt2)
	LSP := $(PRJ_LSP_DIR)/roland1_1m
	MACROS +=  -DROLAND1_1M -DTRIDUCTOR -DZC3750CJQ2 -DSTA_BOARD_2_0_01
	MACROS += -DDDR_MEM_SIZE=0
	PRJ_OBJ_DIR := ./bin/roland1_1m_clt2
	MACROS += -DSTATIC_NODE -DSTD_DUAL 
	CONFIG_MIRACL = y
else ifeq ($(MAKECMDGOALS),roland9_1m_cco)
	LSP := $(PRJ_LSP_DIR)/roland9_1m
	MACROS +=  -DROLAND9_1M -DTRIDUCTOR -DZC3951CCO -DCCO_BOARD_2_0_01 -DCCO_BOARD_5_0_02
	MACROS += -DDDR_MEM_SIZE=8 
	PRJ_OBJ_DIR := ./bin/roland9_1m_cco
	MACROS += -DSTATIC_MASTER -DSTD_DUAL 
	CONFIG_MIRACL = y
	CONFIG_GE_DRV=y
# else ifneq ($(findstring $(MAKECMDGOALS), roland1_1m_romtext), )
# 	LSP := $(PRJ_LSP_DIR)/$(MAKECMDGOALS)
# 	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
# 	MACROS += -DSTD_2016
# 	MACROS += -DASIC_VERSION -DROLAND1_1M
# 	MACROS += -DIDLING_TEXT_IN_ROM
# 	MKIMGFLAGS += -r -v 0x7e000000
else ifneq ($(findstring $(MAKECMDGOALS), mizar9m), )
	LSP := $(PRJ_LSP_DIR)/$(MAKECMDGOALS)
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3951CCO -DSTATIC_MASTER -DSTD_2016 -DSTA_BOARD_2_0_01
	#MACROS += -DSTD_DUAL -DZC3750STA -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_3_0_02  -DSTA_BOARD_2_0_01
	MACROS += -DASIC_VERSION -D$(shell echo $(MAKECMDGOALS) | tr a-z A-Z)
else ifneq ($(findstring $(MAKECMDGOALS), mizar9m_cco), )
	LSP := $(PRJ_LSP_DIR)/mizar9m
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3951CCO -DSTATIC_MASTER -DSTD_2016 -DSTA_BOARD_2_0_01
	MACROS += -DASIC_VERSION  
	MACROS += -DMIZAR9M
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), mizar1m_sta), )
	LSP := $(PRJ_LSP_DIR)/mizar1m
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3750STA -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 -DSTA_BOARD_2_1_00 -DSTA_BOARD_3_0_02 -DSTA_BOARD_5_0_02
	MACROS += -DASIC_VERSION 
	MACROS += -DMIZAR1M
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), mizar1m_clt2), )
	LSP := $(PRJ_LSP_DIR)/mizar1m
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3750CJQ2 -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01
	MACROS += -DASIC_VERSION 
	MACROS += -DMIZAR1M
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), venus8m_cco), )
	LSP := $(PRJ_LSP_DIR)/venus8m
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3951CCO -DSTATIC_MASTER -DSTD_2016 -DSTA_BOARD_2_0_01
	MACROS += -DASIC_VERSION
	MACROS += -DVENUS8M
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), venus2m_sta), )
	LSP := $(PRJ_LSP_DIR)/venus2m
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3750STA -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 -DSTA_BOARD_2_1_00 -DSTA_BOARD_3_0_02 
	MACROS += -DASIC_VERSION 
	MACROS += -DVENUS2M
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), venus2m_clt2), )
	LSP := $(PRJ_LSP_DIR)/venus2m
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3750CJQ2 -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 
	MACROS += -DASIC_VERSION 
	MACROS += -DVENUS2M
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
#####
else ifneq ($(findstring $(MAKECMDGOALS), venus8m_riscv_cco), )
	LSP := $(PRJ_LSP_DIR)/venus8m_riscv/venus_98m.lds
	PRJ_OBJ_DIR2 := ./bin/$(MAKECMDGOALS)
	MACROS2 += -DSTD_DUAL -DZC3951CCO -DSTATIC_MASTER -DSTD_2016 -DSTA_BOARD_2_0_01
	MACROS2 += -DASIC_VERSION
	MACROS2 += -DVENUS8M
	MACROS2 += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), venus2m_riscv_sta), )
	LSP := $(PRJ_LSP_DIR)/venus2m_riscv/venus_18m.lds
	PRJ_OBJ_DIR2 := ./bin/$(MAKECMDGOALS)
	MACROS2 += -DSTD_DUAL -DZC3750STA -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 -DSTA_BOARD_2_1_00 -DSTA_BOARD_3_0_02 
	MACROS2 += -DASIC_VERSION 
	MACROS2 += -DVENUS2M
	MACROS2 += -DMIRACL=32
	CONFIG_MIRACL = y
	MACROS2 += -DIDLING_TEXT_IN_ROM
	MKIMGFLAGS2 += -r -v 0xfe190000
else ifneq ($(findstring $(MAKECMDGOALS), venus2m_riscv_clt2), )
	LSP := $(PRJ_LSP_DIR)/venus2m_riscv/venus_18m.lds
	PRJ_OBJ_DIR2 := ./bin/$(MAKECMDGOALS)
	MACROS2 += -DSTD_DUAL -DZC3750CJQ2 -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 
	MACROS2 += -DASIC_VERSION 
	MACROS2 += -DVENUS2M
	MACROS2 += -DMIRACL=32
	CONFIG_MIRACL = y
	MACROS2 += -DIDLING_TEXT_IN_ROM
	MKIMGFLAGS2 += -r -v 0xfe190000
#####
else ifneq ($(findstring $(MAKECMDGOALS), venus8m_233l_cco), )
	LSP := $(PRJ_LSP_DIR)/venus8m_233l/venus_8m_d233l.lds
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3951CCO -DSTATIC_MASTER -DSTD_2016 -DSTA_BOARD_2_0_01
	MACROS += -DASIC_VERSION
	MACROS += -DVENUS8M
	MACROS += -DV233L_3780
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), venus2m_233l_sta), )
    LSP := $(PRJ_LSP_DIR)/venus2m_233l/venus_1cm_d233l.lds
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3750STA -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 -DSTA_BOARD_2_1_00 -DSTA_BOARD_3_0_02 
	MACROS += -DASIC_VERSION 
	MACROS += -DVENUS2M
	MACROS += -DV233L_3780
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), venus2m_233l_clt2), )
    LSP := $(PRJ_LSP_DIR)/venus2m_233l/venus_1cm_d233l.lds
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3750CJQ2 -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 
	MACROS += -DASIC_VERSION 
	MACROS += -DVENUS2M
	MACROS += -DV233L_3780
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
#####  233l_V3
else ifneq ($(findstring $(MAKECMDGOALS), venus8m_v3_233l_cco), )
	LSP := $(PRJ_LSP_DIR)/venus8m_v3_233l/venus_98m_d233l.lds
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3951CCO -DSTATIC_MASTER -DSTD_2016 -DSTA_BOARD_2_0_01
	MACROS += -DASIC_VERSION
	MACROS += -DVENUS8M -DVENUS_V3
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), venus2m_v3_233l_sta), )
    LSP := $(PRJ_LSP_DIR)/venus2m_v3_233l/venus_romtext_18m_d233l.lds
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3750STA -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 -DSTA_BOARD_2_1_00 -DSTA_BOARD_3_0_02 
	MACROS += -DASIC_VERSION 
	MACROS += -DVENUS2M -DVENUS_V3
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
else ifneq ($(findstring $(MAKECMDGOALS), venus2m_v3_233l_clt2), )
    LSP := $(PRJ_LSP_DIR)/venus2m_v3_233l/venus_romtext_18m_d233l.lds
	PRJ_OBJ_DIR := ./bin/$(MAKECMDGOALS)
	MACROS += -DSTD_DUAL -DZC3750CJQ2 -DSTATIC_NODE -DSTD_2016 -DSTA_BOARD_2_0_01 
	MACROS += -DASIC_VERSION 
	MACROS += -DVENUS2M -DVENUS_V3
	MACROS += -DMIRACL=32
	CONFIG_MIRACL = y
endif

# ifeq ($(MAKECMDGOALS),unicorn2m)
# 	MACROS += -DDDR_MEM_SIZE=2
# else ifeq ($(MAKECMDGOALS),unicorn8m)
# 	MACROS += -DDDR_MEM_SIZE=8
# 	CONFIG_GE_DRV=y
# else ifeq ($(MAKECMDGOALS),roland1_1m)
# 	MACROS += -DDDR_MEM_SIZE=0 
# else ifeq ($(MAKECMDGOALS),roland1_1m_romtext)
# 	MACROS += -DDDR_MEM_SIZE=0 
# else ifeq ($(MAKECMDGOALS),roland9_1m)
# 	MACROS += -DDDR_MEM_SIZE=8 
# else 
ifeq ($(MAKECMDGOALS),mizar9m)
	MACORS += -DDDR_MEM_SIZE=8
else ifeq ($(MAKECMDGOALS),mizar9m_cco)
	MACORS += -DDDR_MEM_SIZE=8
else ifeq ($(MAKECMDGOALS),mizar1m_sta)
	MACORS += -DDDR_MEM_SIZE=0
else ifeq ($(MAKECMDGOALS),mizar1m_clt2)
	MACORS += -DDDR_MEM_SIZE=0
else ifeq ($(MAKECMDGOALS),venus8m_cco)
	MACORS += -DDDR_MEM_SIZE=8
else ifeq ($(MAKECMDGOALS),venus2m_sta)
	MACORS += -DDDR_MEM_SIZE=0
else ifeq ($(MAKECMDGOALS),venus2m_clt2)
	MACORS += -DDDR_MEM_SIZE=0
#####
else ifeq ($(MAKECMDGOALS),venus8m_riscv_cco)
	MACROS2 += -DDDR_MEM_SIZE=8
else ifeq ($(MAKECMDGOALS),venus2m_riscv_sta)
	MACROS2 += -DDDR_MEM_SIZE=0
else ifeq ($(MAKECMDGOALS),venus2m_riscv_clt2)
	MACROS2 += -DDDR_MEM_SIZE=0
#####
else ifeq ($(MAKECMDGOALS),venus8m_233l_cco)
	MACORS += -DDDR_MEM_SIZE=8
else ifeq ($(MAKECMDGOALS),venus2m_233l_sta)
	MACORS += -DDDR_MEM_SIZE=0
else ifeq ($(MAKECMDGOALS),venus2m_233l_clt2)
	MACORS += -DDDR_MEM_SIZE=0
##### v3_233l
else ifeq ($(MAKECMDGOALS),venus8m_v3_233l_cco)
	MACORS += -DDDR_MEM_SIZE=8
else ifeq ($(MAKECMDGOALS),venus2m_v3_233l_sta)
	MACORS += -DDDR_MEM_SIZE=0
else ifeq ($(MAKECMDGOALS),venus2m_v3_233l_clt2)
	MACORS += -DDDR_MEM_SIZE=0
endif

MACROS += -DBOARD_METER

ifeq ($(call qstrip,$(CONFIG_INET_LWIP)), y)
	MACROS += -DINET_LWIP
	MACROS += -DINET_TELNET
	MACROS += -DINET_TFTP
	MACROS += -DLWIP_UDP
endif

ifeq ($(call qstrip,$(CONFIG_GE_DRV)), y)
	MACROS += -DGE_DRV
endif

MACROS += -I $(PRJ_SRC_DIR) -I $(PRJ_SRC_DIR)/trios/include -I $(PRJ_INC_DIR) -I $(PRJ_INC_DIR)/phy  \
	  -I $(PRJ_SRC_DIR)/trios/include/drv/
MACROS2 += -I $(PRJ_SRC_DIR) -I $(PRJ_SRC_DIR)/trios/include -I $(PRJ_INC_DIR) -I $(PRJ_INC_DIR)/phy  \
	  -I $(PRJ_SRC_DIR)/trios/include/drv/
ifeq ($(MAKECMDGOALS),unicorn2m_sta)
LSP := $(PRJ_LSP_DIR)/unicorn2m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/unicorn
else ifeq ($(MAKECMDGOALS),unicorn2m_clt2)
LSP := $(PRJ_LSP_DIR)/unicorn2m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/unicorn
else ifeq ($(MAKECMDGOALS),unicorn8m_cco)
LSP := $(PRJ_LSP_DIR)/unicorn8m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/unicorn
else ifeq ($(MAKECMDGOALS),roland1_1m_sta)
LSP := $(PRJ_LSP_DIR)/roland1_1m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/roland_v2
else ifeq ($(MAKECMDGOALS),roland1_1m_clt2)
LSP := $(PRJ_LSP_DIR)/roland1_1m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/roland_v2
else ifeq ($(MAKECMDGOALS),roland9_1m_cco)
LSP := $(PRJ_LSP_DIR)/roland9_1m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/roland_v2
else ifeq ($(MAKECMDGOALS),mizar9m)
MACROS += -I $(PRJ_INC_DIR)/mizar
else ifeq ($(MAKECMDGOALS),mizar9m_cco)
LSP := $(PRJ_LSP_DIR)/mizar9m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/mizar
else ifeq ($(MAKECMDGOALS),mizar1m_sta)
LSP := $(PRJ_LSP_DIR)/mizar1m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/mizar
else ifeq ($(MAKECMDGOALS),mizar1m_clt2)
LSP := $(PRJ_LSP_DIR)/mizar1m/ldscripts/elf32xtensa.xbn
MACROS += -I $(PRJ_INC_DIR)/mizar
else ifeq ($(MAKECMDGOALS),venus8m_cco)
LSP := $(PRJ_LSP_DIR)/venus8m/venus_8m_d233l.lds
MACROS += -I $(PRJ_INC_DIR)/venus
else ifeq ($(MAKECMDGOALS),venus2m_sta)
LSP := $(PRJ_LSP_DIR)/venus2m/venus_2m_d233l.lds
MACROS += -I $(PRJ_INC_DIR)/venus
else ifeq ($(MAKECMDGOALS),venus2m_clt2)
LSP := $(PRJ_LSP_DIR)/venus2m/venus_2m_d233l.lds
MACROS += -I $(PRJ_INC_DIR)/venus
#######
else ifeq ($(MAKECMDGOALS),venus8m_riscv_cco)
MACROS2 += -I $(PRJ_INC_DIR)/venus
else ifeq ($(MAKECMDGOALS),venus2m_riscv_sta)
MACROS2 += -I $(PRJ_INC_DIR)/venus
else ifeq ($(MAKECMDGOALS),venus2m_riscv_clt2)
MACROS2 += -I $(PRJ_INC_DIR)/venus
#######
else ifeq ($(MAKECMDGOALS),venus8m_233l_cco)
MACROS += -I $(PRJ_INC_DIR)/venus 
else ifeq ($(MAKECMDGOALS),venus2m_233l_sta)
MACROS += -I $(PRJ_INC_DIR)/venus 
else ifeq ($(MAKECMDGOALS),venus2m_233l_clt2)
MACROS += -I $(PRJ_INC_DIR)/venus
####### v3_233l
else ifeq ($(MAKECMDGOALS),venus8m_v3_233l_cco)
MACROS += -I $(PRJ_INC_DIR)/venus_v3 
else ifeq ($(MAKECMDGOALS),venus2m_v3_233l_sta)
MACROS += -I $(PRJ_INC_DIR)/venus_v3 
else ifeq ($(MAKECMDGOALS),venus2m_v3_233l_clt2)
MACROS += -I $(PRJ_INC_DIR)/venus_v3
endif

ifeq ($(call qstrip,$(CONFIG_INET_LWIP)), y)
MACROS  += -I $(PRJ_SRC_DIR)/lwip -I $(PRJ_SRC_DIR)/lwip/include
MACROS2 += -I $(PRJ_SRC_DIR)/lwip -I $(PRJ_SRC_DIR)/lwip/include
else
MACROS  += -I $(PRJ_SRC_DIR)/net
MACROS2 += -I $(PRJ_SRC_DIR)/net
endif
MACROS += -I $(PRJ_SRC_DIR) -I $(PRJ_INC_DIR) -I $(PRJ_MAC_DIR) -I $(PRJ_DATALINK_DIR)  -I $(PRJ_APP_DIR) -I $(PRJ_SI4438_DIR) -I $(PRJ_SCH_DIR) -I $(PRJ_BSP_DIR)
MACROS2 += -I $(PRJ_SRC_DIR) -I $(PRJ_INC_DIR) -I $(PRJ_MAC_DIR) -I $(PRJ_DATALINK_DIR)  -I $(PRJ_APP_DIR) -I $(PRJ_SI4438_DIR) -I $(PRJ_SCH_DIR) -I $(PRJ_BSP_DIR)
ifeq ($(call qstrip, $(CONFIG_MIRACL)), y)
MACROS  += -I $(PRJ_SRC_DIR)/crypto -I $(PRJ_SRC_DIR)/crypto/miracl/include
MACROS2 += -I $(PRJ_SRC_DIR)/crypto -I $(PRJ_SRC_DIR)/crypto/miracl/include
endif

ifeq ($(call qstrip, $(CONFIG_XTENSA_LINUX_GCC)), y)
CFLAGS := -g -Wall -Werror -O1 $(MACROS) -c -ffunction-sections -fdata-sections
LINK_FLAG = -nostdlib -T$(LSP) -L $(NEWLIB_PATH)/lib -lc -lgcc -lhandlers-board -lhal -Wl,--gc-sections
else
CFLAGS := -c -g -Wall -Werror -O0 -INLINE:requested -mlongcalls -Wa,--gdwarf2 $(MACROS) -mlsp=$(LSP) -ffunction-sections -fdata-sections
endif

ifeq ($(call qstrip, $(CONFIG_RISCV_LINUX_GCC)), y)
CFLAGS2    := -g -Wall -Werror -O0 -INLINE:requested -Wa,--gdwarf2 $(MACROS2) -c -ffunction-sections -fdata-sections
LINK_FLAG2 := -nostartfiles -T$(LSP) -Wl,--gc-sections
endif

###########################################################
#
#	the whole directory path
#
###########################################################
LIB += -lm
EX_DIRS := $(shell find `find ${PRJ_SRC_DIR} -name include -o -name CVS -o -name make -o -name lsp -type d` -type d)
ifeq ($(call qstrip,$(CONFIG_INET_LWIP)), y)
	EX_DIRS += $(call qfind, $(PRJ_SRC_DIR), net)
else
	EX_DIRS += $(call qfind, $(PRJ_SRC_DIR), lwip)
endif

ifneq ($(call qstrip,$(CONFIG_GE_DRV)), y)
	EX_SRCS += ge.c
	EX_DIRS += $(call qfind,$(PRJ_SRC_DIR),eth_lan)
endif

ifneq ($(call qstrip, $(CONFIG_MIRACL)), y)
	EX_DIRS += $(call qfind,$(PRJ_SRC_DIR),crypto)
endif

DIRS := $(shell find $(PRJ_SRC_DIR) -type d)
DIRS := $(filter-out $(EX_DIRS),$(DIRS))
ALL_PATH = $(DIRS)
VPATH := $(ALL_PATH)

###########################################################
#
#	generate the target files and dependence file
#
###########################################################

ALL_C_FILES  := $(foreach i, $(ALL_PATH), $(wildcard $(i)/*.c))
ALL_C_FILES  := $(notdir $(ALL_C_FILES))
ALL_C_FILES  := $(filter-out $(EX_SRCS), $(ALL_C_FILES))
ALL_O_FILES  := $(patsubst %.c,%.o,$(ALL_C_FILES))
ALL_O_FILES  := $(addprefix $(PRJ_OBJ_DIR)/,$(ALL_O_FILES))

ALL_C_FILES2 := $(foreach i, $(ALL_PATH), $(wildcard $(i)/*.c))
ALL_C_FILES2 := $(notdir $(ALL_C_FILES2))
ALL_C_FILES2 := $(filter-out $(EX_SRCS), $(ALL_C_FILES))
ALL_O_FILES2 := $(patsubst %.c,%.o,$(ALL_C_FILES2))
ALL_O_FILES2 := $(addprefix $(PRJ_OBJ_DIR2)/,$(ALL_O_FILES2))

$(shell mkdir -p bin)
$(shell mkdir -p $(PRJ_OBJ_DIR))
$(shell mkdir -p $(PRJ_OBJ_DIR2))

ifeq ($(MAKECMDGOALS),unicorn2m_sta)
PROJ	      := image_unicorn2m_sta
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/unicorn2m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/unicorn2m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/unicorn2m/libphy.a
else ifeq ($(MAKECMDGOALS),unicorn2m_clt2)
PROJ	      := image_unicorn2m_clt2
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/unicorn2m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/unicorn2m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/unicorn2m/libphy.a
else ifeq ($(MAKECMDGOALS),unicorn8m_cco)
PROJ	      := image_unicorn8m_cco
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/unicorn8m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/unicorn8m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/unicorn8m/libphy.a
GE_LIB        := $(PRJ_LIB_DIR)/unicorn8m/libge.a
else ifeq ($(MAKECMDGOALS),roland1_1m_sta)
PROJ	      := image_roland1_1m_sta
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/roland1_1m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/roland1_1m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/roland1_1m/libphy.a
else ifeq ($(MAKECMDGOALS),roland1_1m_clt2)
PROJ	      := image_roland1_1m_clt2
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/roland1_1m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/roland1_1m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/roland1_1m/libphy.a
else ifeq ($(MAKECMDGOALS),roland9_1m_cco)
PROJ	      := image_roland9_1m_cco
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/roland9_1m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/roland9_1m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/roland9_1m/libphy.a
GE_LIB        := $(PRJ_LIB_DIR)/roland9_1m/libge.a
# GE_LIB        := $(PRJ_LIB_DIR)/roland9_1m/libcrypto.a

#mizar#
else ifeq ($(MAKECMDGOALS),mizar9m)
PROJ	      := image_mizar9m
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/mizar9m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/mizar9m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/mizar9m/libphy.a
else ifeq ($(MAKECMDGOALS),mizar9m_cco)
PROJ	      := image_mizar9m_cco
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/mizar9m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/mizar9m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/mizar9m/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/mizar9m/libcrypto.a
else ifeq ($(MAKECMDGOALS),mizar1m_sta)
PROJ	      := image_mizar1m_sta
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/mizar1m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/mizar1m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/mizar1m/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/mizar1m/libcrypto.a
else ifeq ($(MAKECMDGOALS),mizar1m_clt2)
PROJ	      := image_mizar1m_clt2
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/mizar1m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/mizar1m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/mizar1m/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/mizar1m/libcrypto.a
#venus#
else ifeq ($(MAKECMDGOALS),venus8m_cco)					
PROJ	      := image_venus8m_cco
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus8m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus8m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus8m/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus8m/libcrypto.a
else ifeq ($(MAKECMDGOALS),venus2m_sta)
PROJ	      := image_venus2m_sta
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus2m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus2m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus2m/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus2m/libcrypto.a
else ifeq ($(MAKECMDGOALS),venus2m_clt2)
PROJ	      := image_venus2m_clt2
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus2m/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus2m/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus2m/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus2m/libcrypto.a
#venus_riscv#
else ifeq ($(MAKECMDGOALS),venus8m_riscv_cco)
PROJ2	      := image_venus8m_riscv_cco
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus8m_riscv/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus8m_riscv/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus8m_riscv/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus8m_riscv/libcrypto.a
else ifeq ($(MAKECMDGOALS),venus2m_riscv_sta)
PROJ2	      := image_venus2m_riscv_sta
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus2m_riscv/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus2m_riscv/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus2m_riscv/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus2m_riscv/libcrypto.a
else ifeq ($(MAKECMDGOALS),venus2m_riscv_clt2)
PROJ2	      := image_venus2m_riscv_clt2
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus2m_riscv/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus2m_riscv/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus2m_riscv/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus2m_riscv/libcrypto.a
#venus_2331#
else ifeq ($(MAKECMDGOALS),venus8m_233l_cco)
PROJ	      := image_venus8m_233l_cco
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus8m_233l/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus8m_233l/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus8m_233l/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus8m_233l/libcrypto.a
else ifeq ($(MAKECMDGOALS),venus2m_233l_sta)
PROJ	      := image_venus2m_233l_sta
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus2m_233l/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus2m_233l/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus2m_233l/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus2m_233l/libcrypto.a
else ifeq ($(MAKECMDGOALS),venus2m_233l_clt2)
PROJ	      := image_venus2m_233l_clt2
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus2m_233l/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus2m_233l/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus2m_233l/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus2m_233l/libcrypto.a
#venus_v3_2331#
else ifeq ($(MAKECMDGOALS),venus8m_v3_233l_cco)
PROJ	      := image_venus8m_v3_233l_cco
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus8m_v3_233l/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus8m_v3_233l/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus8m_v3_233l/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus8m_v3_233l/libcrypto.a
else ifeq ($(MAKECMDGOALS),venus2m_v3_233l_sta)
PROJ	      := image_venus2m_v3_233l_sta
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus2m_v3_233l/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus2m_v3_233l/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus2m_v3_233l/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus2m_v3_233l/libcrypto.a
else ifeq ($(MAKECMDGOALS),venus2m_v3_233l_clt2)
PROJ	      := image_venus2m_v3_233l_clt2
OS_KERNEL_LIB := $(PRJ_LIB_DIR)/venus2m_v3_233l/libos.a
OS_CPU_LIB    := $(PRJ_LIB_DIR)/venus2m_v3_233l/libcpu.a
PHY_LIB       := $(PRJ_LIB_DIR)/venus2m_v3_233l/libphy.a
CRYPTO_LIB    := $(PRJ_LIB_DIR)/venus2m_v3_233l/libcrypto.a
else
PROJ          := error
endif

default: help

###########################################################
#
#	to generate proj
#
###########################################################
.PHONY: all roland1_1m roland1_1m_romtext roland9_1m unicorn2m unicorn8m mizar9m mizar9m_cco mizar1m_sta mizar1m_clt2
all:
	#make -j roland1_1m
	#make -j roland1_1m_romtext
	#make -j roland9_1m
	#make -j unicorn2m
	#make -j unicorn8m
	#make -j mizar9m
	#make -j mizar9m_cco
	#make -j mizar1m_sta
	#make -j mizar1m_clt2
	make -j venus8m_cco
	make -j venus2m_sta
	make -j venus2m_clt2
	
	make -j venus8m_riscv_cco
	make -j venus2m_riscv_sta
	make -j venus2m_riscv_clt2

	make -j venus8m_233l_cco
	make -j venus2m_233l_sta
	make -j venus2m_233l_clt2
	
	#v3_233l
	make -j venus8m_v3_233l_cco
	make -j venus2m_v3_233l_sta
	make -j venus2m_v3_233l_clt2

	#unicorn
	make -j unicorn2m_sta
	make -j unicorn2m_clt2
	make -j unicorn8m_cco

	#roland
	make -j roland1_1m_sta
	make -j roland1_1m_clt2
	make -j roland9_1m_cco
	


unicorn2m_sta:$(PROJ)
unicorn2m_clt2:$(PROJ)
unicorn8m_cco:$(PROJ)
roland1_1m_sta:$(PROJ)
roland1_1m_clt2:$(PROJ)
roland9_1m_cco:$(PROJ)

mizar9m:$(PROJ)
mizar9m_cco:$(PROJ)
mizar1m_sta:$(PROJ)
mizar1m_clt2:$(PROJ)
venus8m_cco:$(PROJ)
venus2m_sta:$(PROJ)
venus2m_clt2:$(PROJ)

venus8m_riscv_cco:$(PROJ2)
venus2m_riscv_sta:$(PROJ2)
venus2m_riscv_clt2:$(PROJ2)

venus8m_233l_cco:$(PROJ)
venus2m_233l_sta:$(PROJ)
venus2m_233l_clt2:$(PROJ)

#v3_233l
venus8m_v3_233l_cco:$(PROJ)
venus2m_v3_233l_sta:$(PROJ)
venus2m_v3_233l_clt2:$(PROJ)

$(PROJ): $(ALL_O_FILES) $(OS_KERNEL_LIB) $(OS_CPU_LIB) $(GE_LIB) $(PHY_LIB) $(MAC_LIB) $(CRYPTO_LIB)
	$(CC) $(CFLAGS:-c=) $(ALL_O_FILES) -o $@ -Wl,-whole-archive $(OS_CPU_LIB) -Wl,-no-whole-archive $(GE_LIB) $(PHY_LIB) $(MAC_LIB) $(CRYPTO_LIB) $(OS_KERNEL_LIB) $(LIB) $(LINK_FLAG)
	$(MKIMG) $(MKIMGFLAGS) $(PROJ) -o $(PROJ).bin
	$(call done)

$(PRJ_OBJ_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $< -o $@

$(PRJ_OBJ_DIR)/%.d: %.S
	$(CC) $(MACROS) -I $(PRJ_SRC_DIR) -MM $< >$@
	$(_P)$(CAT) $@ > $@.tmp
	$(_P)$(RM) $@
	$(_P)$(SED) -e 's|.*:|$(PRJ_OBJ_DIR)\/$(notdir $(patsubst %.d,%.o,$@)):|' < $@.tmp > $@
	$(_P)$(RM) $@.tmp
$(PROJ2): $(ALL_O_FILES2) $(OS_CPU_LIB) $(CRYPTO_LIB) $(OS_KERNEL_LIB) $(PHY_LIB)
	$(CC2) $(CFLAGS2:-c=) $(ALL_O_FILES2) -o $@ -Wl,-whole-archive $(OS_CPU_LIB) -Wl,-no-whole-archive $(PHY_LIB) $(CRYPTO_LIB) $(OS_KERNEL_LIB) $(LIB) $(LINK_FLAG2)
	$(MKIMG) $(MKIMGFLAGS2) $(PROJ2) -o $(PROJ2).bin
	$(call done)

$(PRJ_OBJ_DIR2)/%.o:%.c
	$(CC2) $(CFLAGS2) $< -o $@
###########################################################
#
#	unicorn
#
###########################################################

.PHONY: unicorn 
unicorn:
	make -j unicorn2m_sta
	make -j unicorn2m_clt2
	make -j unicorn8m_cco
	
###########################################################
#
#	roland
#
###########################################################
.PHONY: roland 
roland:
	make -j roland1_1m_sta
	make -j roland1_1m_clt2
	make -j roland9_1m_cco
	

###########################################################
#
#	mizar
#
###########################################################

.PHONY: mizar 
mizar:
	make -j mizar9m_cco
	make -j mizar1m_sta
	#make -j mizar1m_clt2
	
.PHONY: venus 
venus:
	make -j venus8m_cco
	make -j venus2m_sta
	make -j venus2m_clt2
	
	
.PHONY: venus_riscv 
venus_riscv:
	make -j venus8m_riscv_cco
	make -j venus2m_riscv_sta
	make -j venus2m_riscv_clt2
	
.PHONY: venus_233l 
venus_233l:
	make -j venus8m_233l_cco
	make -j venus2m_233l_sta
	make -j venus2m_233l_clt2

#v3_233l	
.PHONY: venus_v3_233l 
venus_v3_233l:
	make -j venus8m_v3_233l_cco
	make -j venus2m_v3_233l_sta
	make -j venus2m_v3_233l_clt2
	

###########################################################
#
#	do clean
#
###########################################################
.PHONY: clean
clean:
	$(_P)$(RM) $(PRJ_OBJ_DIR)/*
	$(_P)$(RM) $(PRJ_OBJ_DIR2)/*
	$(_P)$(RM) image_*
	$(call done)


###########################################################
#
#	show help
#
###########################################################
.PHONY: help
help:
	$(_P)echo -e "Usage: make [options]"
	$(_P)echo -e "Options:"
	$(_P)echo -e "\tall\t--- generate image"
	$(_P)echo -e "\tunicorn2m_sta\t--- generate image_unicorn2m_sta"
	$(_P)echo -e "\tunicorn2m_cjq2\t--- generate image_unicorn2m_cjq2"
	$(_P)echo -e "\tunicorn8m_cco\t--- generate image_unicorn8m_cco"
	$(_P)echo -e "\troland1_1m_sta\t--- generate image_roland1_1m_sta"
	$(_P)echo -e "\troland1_1m_cjq2\t--- generate image_roland1_1m_cjq2"
	$(_P)echo -e "\troland9_1m_cco\t--- generate image_roland9_1m_cco"

	$(_P)echo -e "\tmizar9m_cco\t--- generate image_mizar9m_cco"
	$(_P)echo -e "\tmizar1m_sta\t--- generate image_mizar1m_sta"
	$(_P)echo -e "\tmizar1m_clt2\t--- generate image_mizar1m_clt2"
	$(_P)echo -e "\tvenus8m_cco\t--- generate image_vensu8m_cco"
	$(_P)echo -e "\tvenus2m_sta\t--- generate image_vensu2m_sta"
	$(_P)echo -e "\tvenus2m_ctl2\t--- generate image_vensu2m_ctl2"
	$(_P)echo -e "\thelp\t--- help imformation"

shchange:
	sed -i 's/\r//g'  *.sh
	
	
	