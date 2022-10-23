sm := ta_arm32
sm-ta_arm32 := y
CFG_TA_FLOAT_SUPPORT := y
CFG_ARM32_ta_arm32 := y
ta_arm32-platform-cppflags := -DARM32=1 -D__ILP32__=1 -DMBEDTLS_SELF_TEST
ta_arm32-platform-cflags := -mcpu=cortex-a9 -Os -g3 -fpie -mthumb -mthumb-interwork -fno-short-enums -fno-common -mno-unaligned-access -fstack-protector-strong -mfloat-abi=hard -funsafe-math-optimizations
ta_arm32-platform-cxxflags := -mcpu=cortex-a9 -Os -g3 -fpie -mthumb -mthumb-interwork -fno-short-enums -fno-common -mno-unaligned-access -fstack-protector-strong -mfloat-abi=hard -funsafe-math-optimizations
ta_arm32-platform-aflags := -mcpu=cortex-a9
CFG_TA_MBEDTLS_SELF_TEST := y
CFG_TA_MBEDTLS := y
CFG_TA_MBEDTLS := y
CROSS_COMPILE32 ?= $(CROSS_COMPILE)
CROSS_COMPILE_ta_arm32 ?= $(CROSS_COMPILE32)

