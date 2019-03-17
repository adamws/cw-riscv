TARGET = cw-riscv
C_SRCS += main.c ./tiny-AES-c/aes.c
CFLAGS += -O2 -fno-builtin-printf -DNO_INIT

HEADERS := ./tiny-AES-c/aes.h

BSP_BASE = ../../bsp
include $(BSP_BASE)/env/common.mk
