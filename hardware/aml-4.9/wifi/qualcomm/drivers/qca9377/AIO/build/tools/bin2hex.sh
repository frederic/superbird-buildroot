#!/bin/sh

FW_BIN_INPUT=$1
FW_H_OUTPUT=$2
FW_ARRAY_NAME=$3
perl ./bin2hex.pl c ${FW_BIN_INPUT} ${FW_H_OUTPUT}.tmp arr_form 16
sed 's/data/'${FW_ARRAY_NAME}'/' ${FW_H_OUTPUT}.tmp  > ${FW_H_OUTPUT}
rm -rf ${FW_H_OUTPUT}.tmp
