CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

OBJS = crypto_perf.o crypto_cmd.o

ifeq ($(ARCH), arm64)
TEEC_EXPORT=$(TDK_DIR)/ca_export_arm64
endif

ifeq ($(ARCH), arm)
TEEC_EXPORT=$(TDK_DIR)/ca_export_arm
endif

TA_DEV_KIT_DIR=$(TDK_DIR)/ta_export
CFLAGS += -Wall -I../ta/include -I$(TEEC_EXPORT)/include -I./include
CFLAGS += -I$(TA_DEV_KIT_DIR)/host_include
CFLAGS += -I../../optee_test/ta/crypt/include/
#Add/link other required libraries here
LDADD += -lteec -L$(TEEC_EXPORT)/lib

BINARY=tee_crypto_perf

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	mkdir -p $(O)
	$(CC) $(LDADD) -o $(O)/$@ $+

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)
