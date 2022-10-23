TDK_TOP_DIR := $(call my-dir)
TA_SIGN_TOOL := $(TDK_TOP_DIR)/ta_export/scripts/sign_ta.py
TA_GEN_CERT_TOOL := $(TDK_TOP_DIR)/ta_export/scripts/gen_cert_key.py
TA_ROOT_PRIV_KEY ?= $(TDK_TOP_DIR)/ta_export/keys/root_rsa_prv_key.pem
TA_USER_PUB_KEY ?= $(TDK_TOP_DIR)/ta_export/keys/ta_rsa_pub_key.pem
TA_USER_PRIV_KEY ?= $(TDK_TOP_DIR)/ta_export/keys/ta_rsa_prv_key.pem
TA_ROOT_AES_KEY ?= $(TDK_TOP_DIR)/ta_export/keys/root_aes_key.bin
TA_USER_AES_KEY ?= $(TDK_TOP_DIR)/ta_export/keys/ta_aes_key.bin
TA_USER_AES_IV ?= $(TDK_TOP_DIR)/ta_export/keys/ta_aes_iv.bin

include $(call all-subdir-makefiles)
