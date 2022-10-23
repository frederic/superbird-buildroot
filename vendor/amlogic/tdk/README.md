## How to build Linux 32-bit CA
$ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-


## How to build Linux 64-bit CA
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-


## How to build Linux driver
$ export KERNEL_SRC_DIR=/path/to/your/kernel/source/directory
$ export KERNEL_OUT_DIR=/path/to/your/kernel/output/directory
$ make driver

## How to build Android CA/driver
$ mm

## How to sign TA
$ ./ta_export/scripts/gen_cert_key.py
	--root_rsa_key=ta_export/keys/root_rsa_prv_key.pem
	--ta_rsa_key=ta_export/keys/ta_rsa_pub_key.pem
	--uuid=8aaaf200-2450-11e4-abe2-0002a5d5c51b
	--ta_rsa_key_sig=ta_rsa_key.sig
	--root_aes_key=ta_export/keys/root_aes_key.bin
	--ta_aes_key=ta_export/keys/ta_aes_key.bin
	--ta_aes_iv=ta_export/keys/ta_aes_iv.bin
	--ta_aes_key_iv_enc=ta_aes_key_enc.bin
$ ./ta_export/scripts/sign_ta.py
	--ta_rsa_key=ta_export/keys/ta_rsa_prv_key.pem
	--ta_rsa_key_sig=ta_rsa_key.sig
	--ta_aes_key=ta_export/keys/ta_aes_key.bin
	--ta_aes_iv=ta_export/keys/ta_aes_iv.bin
	--ta_aes_key_iv_enc=ta_aes_key_enc.bin
	--in=8aaaf200-2450-11e4-abe2-0002a5d5c51b.ta
	--out=8aaaf200-2450-11e4-abe2-0002a5d5c51b.ta
