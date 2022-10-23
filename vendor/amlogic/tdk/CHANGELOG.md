# AMLOGIC TDK CHANGELOG

## TDK version 2.4
	* Based on OPTEE v2.4.0
	* Support TXLX platform
	* Correct HDCP status on TV platform
	* Support BL31 REE/TEE shared memory
	* Support multi codec sessions
	* Enable HW SHA on TXLX platform
	* Update TA sign/verify script
	* Support TA encryption
	* Support map/unmap vdec memory dynamically
	* Return error if RPMB key is not burnt
	* Remove API TEE_Setting_Device()

## TDK version 2.3
	* Based on OPTEE v2.3.0

## TDK version 2.2
	* Based on OPTEE v2.2.0
	* Refactor vdec memory mapping API
	* Refactor Meson TEE extension API
	* Update Linux driver to support kernel v4.9
	* Remove libteec.a

## TDK version 2.0.2
	* Based on OPTEE v2.1.0
	* Support TXLX platform
	* Fix RPMB tee_rpmb_free()
	* Support BL31 shared memory 0x05200000
	* Support multi codec sessions
	* Enable HW SHA1/SHA224/SHA256 on TXLX

## TDK version 2.0.1
	* Based on OPTEE v2.0.0
	* Update sign script exit if sign repeatly
	* Export root_rsa_pub_key.pem
	* Support signing TA automaticlly if Secure Boot enabled
