TARGET = cw-riscv
C_SRCS += main.c
CFLAGS += -O2 -fno-builtin-printf -DNO_INIT

BSP_BASE = ../../bsp
include $(BSP_BASE)/env/common.mk
