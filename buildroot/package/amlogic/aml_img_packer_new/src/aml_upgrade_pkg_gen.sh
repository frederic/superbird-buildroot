#!/bin/sh


#only one input param, output signed bootloader and efuse patten
#param1 is not signed u-boot.bin
aml_secureboot_sign_bootloader(){
	echo -----aml-secureboot-sign-bootloader ------
	printf "${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --bootsig "
    printf " --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY} "
    printf " --aeskey enable"
    printf " --input $1 "
    printf " --output $1.encrypt\n"
	${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --bootsig \
        --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY} \
        --aeskey enable \
        --input $1 \
        --output $1.encrypt
	echo ----- Made aml secure-boot singed bootloader: $1.encrypt --------
}

#input para is boot.img or recovery.img
aml_secureboot_sign_kernel(){
	echo -----aml-secureboot-sign-kernel ------
	printf "${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --imgsig"
    printf " --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY}"
    printf " --input $1 --output ${1}.encrypt\n"
	${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --imgsig \
        --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY}  \
        --input $1 --output ${1}.encrypt
	echo ----- Made aml secure-boot singed kernel img: $1.encrypt --------
}
#input para is boot.img or recovery.img
aml_secureboot_sign_kernel_m8b(){
	echo -----aml-secureboot-sign-kernel ------
	${PRODUCT_AML_SECUREBOOT_SIGNTOOL}  \
		${BINARIES_DIR}/aml-rsa-key.k2a \
		${PRODUCT_AML_IMG_PACK_DIR}/boot.img \
		${PRODUCT_AML_IMG_PACK_DIR}/boot.img.encrypt \
		${BINARIES_DIR}/aml-aes-key.aes
	echo ----- Made aml secure-boot singed kernel img: boot.img.encrypt --------
}
aml_secureboot_sign_bin(){
	echo -----aml-secureboot-sign-bin ------
	printf "${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --binsig"
    printf " --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY}"
    printf " --input $1 --output ${1}.encrypt\n"
	${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --binsig \
        --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY}  \
        --input $1 --output ${1}.encrypt
	echo ----- Made aml secure-boot singed bin: $1.encrypt --------
}

platform=$1


if [ $# -ne 3 ]; then
    secureboot=$2
    absystem=$3
else
    if [ "$2" != "y" ]; then
	    absystem=$2
	else
	    secureboot=$2
	fi
fi

PRODUCT_AML_IMG_PACK_TOOL=${TOOL_DIR}/aml_image_v2_packer_new
PRODUCT_AML_IMG_SIMG_TOOL=${TOOL_DIR}/img2simg
PRODUCT_AML_IMG_PACK_DIR=${BINARIES_DIR}
PRODUCT_LOGO_IMG_PACK_TOOL=${TOOL_DIR}/res_packer
PRODUCT_LOGO_IMG_PACK_DIR=${BINARIES_DIR}/logo_img_files
PRODUCT_LOGO_IMG_PACK=${BINARIES_DIR}/logo.img
IMAGE_UBIFS=${BINARIES_DIR}/rootfs.ubifs
IMAGE_UBI=${BINARIES_DIR}/rootfs.ubi

echo "${PRODUCT_LOGO_IMG_PACK_TOOL} -r ${PRODUCT_LOGO_IMG_PACK_DIR} ${PRODUCT_LOGO_IMG_PACK}"
${PRODUCT_LOGO_IMG_PACK_TOOL} -r ${PRODUCT_LOGO_IMG_PACK_DIR} ${PRODUCT_LOGO_IMG_PACK}
if [ $? -ne 0 ]; then
    echo fail to generate logo image;
    rm ${PRODUCT_LOGO_IMG_PACK}
fi
####Step 1: rename dtb to dtb.img
#change gxb_p200.db to dtb.img
#Note: if you using more than one dtb, using 'vendor/amlogic/tools/dtbTool' to pack all your dtbs into the dtb.img
#echo "move your dtbs to add dtb.img"
#ln -sf `ls ${PRODUCT_AML_IMG_PACK_DIR}/*.dtb` ${PRODUCT_AML_IMG_PACK_DIR}/dtb.img

####Step 2: compress 1g
echo PRODUCT_AML_IMG_PACK_DIR:${PRODUCT_AML_IMG_PACK_DIR}
if [ -f ${IMAGE_UBIFS} ]; then
	echo -e "\n !!!!!! use ubifs \n"
	update_sparse_img=0
else

	ext4img=${PRODUCT_AML_IMG_PACK_DIR}/rootfs.ext2
	sparseimg=${ext4img}.img2simg
	update_sparse_img=0
       echo -e " \n\n !!!!!! use ext4 \n\n"
       if [ ! -f  ${sparseimg} ]; then update_sparse_img=1; fi
       if [ ${update_sparse_img} -ne 1 ];then
           t1=`stat -c %Y ${ext4img}`
           t2=`stat -c %Y ${sparseimg}`
           if [ ${t1} -gt ${t2} ]; then
               echo "ext4 file time newer than sparse image"
               update_sparse_img=1
           fi
       fi
fi
if [ ${update_sparse_img} -eq 1 ]; then
    echo "compress 1g ext4 image to compressed sparse format"
    ${PRODUCT_AML_IMG_SIMG_TOOL} ${ext4img} ${sparseimg}
    if [ ! -f ${sparseimg} ]; then
        echo "fail to compress ext4 img to sparse format"
        exit 1
    fi
fi
####Step 3: pack none-secureboot burning image
if [ -f ${IMAGE_UBIFS} ]
then
	if [ "${absystem}" != "absystem" ]; then
	        aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package.conf
	else
		    aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package_ab.conf
	fi
else
	if [ "${absystem}" != "absystem" ]; then
		aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package_emmc.conf
		if [ -f ${aml_upgrade_package_conf} ]; then
			echo aml_upgrade_package_conf=${aml_upgrade_package_conf}
		else
			aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package.conf
			echo aml_upgrade_package_conf=${aml_upgrade_package_conf}
		fi
	else
		aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package_emmc_ab.conf
		if [ -f ${aml_upgrade_package_conf} ]; then
			echo aml_upgrade_package_conf=${aml_upgrade_package_conf}
		else
			aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package.conf
			echo aml_upgrade_package_conf=${aml_upgrade_package_conf}
		fi
	fi
fi

burnPkg=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package.img
if [ "${secureboot}" != "y" ]; then
    echo "generate noraml burning package"
    echo "${PRODUCT_AML_IMG_PACK_TOOL} -r ${aml_upgrade_package_conf} ${PRODUCT_AML_IMG_PACK_DIR} ${burnPkg} "
    ${PRODUCT_AML_IMG_PACK_TOOL} -r ${aml_upgrade_package_conf} ${PRODUCT_AML_IMG_PACK_DIR} ${burnPkg}
    if [ $? -ne 0 ]; then
        echo fail to generate burning image;
        rm ${burnPkg}
    fi
    exit $?
fi

####Step 4: pack secureboot burning image
if [ -f ${IMAGE_UBIFS} ]
then
	aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package_enc.conf
else
	aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package_emmc_enc.conf
fi
burnPkgenc=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package_enc.img
#########Support compiling out encrypted zip/aml_upgrade_package.img directly
#PRODCUT_AML_BOOTLOADER_PATH=./output/build/uboot-custom
PRODUCT_AML_SECUREBOOT_USERKEY=${BINARIES_DIR}/aml-user-key.sig
if [ ${platform} = "meson8b" ];then
	PRODUCT_AML_SECUREBOOT_SIGNTOOL=${TOOL_DIR}/aml_encrypt_m8b
else
	PRODUCT_AML_SECUREBOOT_SIGNTOOL=${TOOL_DIR}/aml_encrypt_${platform}
fi

PRODUCT_OUTPUT_DIR=${BINARIES_DIR}
aml_bootloader=${PRODUCT_OUTPUT_DIR}/u-boot.bin
if [ ${platform} != "meson8b" ];then
	aml_secureboot_sign_bootloader ${aml_bootloader}
	if [ ! -f ${aml_bootloader}.encrypt ]; then
		echo "fail to sign bootloader -----"
		cp ${PRODUCT_OUTPUT_DIR}/u-boot.bin ${aml_bootloader}.encrypt
#    	exit 1
	fi
fi
#rename efuse patten name for windows USB_BURNING_TOOL
mv ${aml_bootloader}.aml.efuse ${PRODUCT_AML_IMG_PACK_DIR}/SECURE_BOOT_SET


aml_kernel=${PRODUCT_OUTPUT_DIR}/boot.img
if [ ${platform} = "meson8b" ];then
	aml_secureboot_sign_kernel_m8b ${aml_kernel}
	if [ ! -f ${aml_kernel}.encrypt ]; then
		echo "fail to sign kernel image"
		exit 1
	fi
else
	aml_secureboot_sign_kernel ${aml_kernel}
	if [ ! -f ${aml_kernel}.encrypt ]; then
		echo "fail to sign kernel image"
		exit 1
	fi
fi

aml_recovery=${PRODUCT_OUTPUT_DIR}/recovery.img
if [ ${platform} = "meson8b" ];then
    aml_secureboot_sign_kernel_m8b ${aml_recovery}
    if [ ! -f ${aml_recovery}.encrypt ]; then
        echo "fail to sign recovery image"
        exit 1
    fi
else
    aml_secureboot_sign_kernel ${aml_recovery}
    if [ ! -f ${aml_recovery}.encrypt ]; then
        echo "fail to sign recovery image"
        exit 1
    fi
fi

aml_dtb=${PRODUCT_OUTPUT_DIR}/dtb.img
if [ ${platform} != "meson8b" ];then
	aml_secureboot_sign_bin ${aml_dtb}
	if [ ! -f ${aml_dtb}.encrypt ]; then
		echo "fail to sign dtb image"
		exit 1
	fi
fi

echo "${PRODUCT_AML_IMG_PACK_TOOL} -r ${aml_upgrade_package_conf} ${PRODUCT_AML_IMG_PACK_DIR} ${burnPkgenc} "
${PRODUCT_AML_IMG_PACK_TOOL} -r ${aml_upgrade_package_conf} ${PRODUCT_AML_IMG_PACK_DIR} ${burnPkgenc}
if [ $? -ne 0 ]; then
    echo fail to generate burning image;
    rm ${burnPkgenc}
fi

exit $?
