#!/bin/bash
#set -x
CONTAINER_VER="1.0"
PRODUCT_NAME="aml-software"
#This is new path from buildroot-2019.02
OTA_SRC_PATH=../build/buildroot-fs/cpio/target_ota

cd ${BINARIES_DIR}

source ota-package-filelist

TARGET_OTA_NAME="target_ota_$(date +%y%m%d%H%M)"
rm $TARGET_OTA_NAME $TARGET_OTA_NAME.zip -rf
mkdir $TARGET_OTA_NAME
cp $HASH_FILES $TARGET_OTA_NAME
rm $TARGET_OTA_NAME/rootfs.*
tar -C $OTA_SRC_PATH -czf $TARGET_OTA_NAME/rootfs.tgz .

cp sw-description-increment $TARGET_OTA_NAME/sw-description

cp increment_update.sh $TARGET_OTA_NAME/update.sh
cp swupdate-priv.pem  $TARGET_OTA_NAME
cp increment.sh  $TARGET_OTA_NAME

cd $TARGET_OTA_NAME
zip -qry ../$TARGET_OTA_NAME.zip .
cd -
#zip -r $TARGET_OTA_NAME.zip $TARGET_OTA_NAME
rm $TARGET_OTA_NAME -rf

for i in $HASH_FILES;do
	value_tmp=$(sha256sum $i);
	value_sha256=${value_tmp:0:64};
	key_value="sha256 = \""$value_sha256"\"";
	sed -i "/filename = \"$i\";/a\\\t\t\t$key_value"  sw-description;
done

openssl dgst -sha256 -sign swupdate-priv.pem sw-description > sw-description.sig
for i in $FILES;do
	echo $i;done | cpio -ov -H crc >  software.swu
echo software.swu  | cpio -ov -H crc >  ${PRODUCT_NAME}_${CONTAINER_VER}.swu
#cd -
