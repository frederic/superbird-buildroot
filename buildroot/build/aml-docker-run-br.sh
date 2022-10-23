#!/bin/bash
# Start container and start process inside container.
#
# Example:
#   ./run.sh            Start a sh shell inside container.
#   ./run.sh ls -la     Run `ls -la` inside container.
#
# Calls to `make` are intercepted and the "O=/buildroot_output" is added to
# command. So calling `./run.sh make savedefconfig` will run `make
# savedefconfig O=/buildroot_output` inside the container.
#
# Example:
#   ./run.sh make       Run `make O=/buildroot_output` in container.
#   ./run.sh make docker_python2_defconfig menuconfig
#                       Build config based on docker_python2_defconfig.
#
# When working with Buildroot you probably want to create a config, build
# some products based on that config and save the config for future use.
# Your workflow will look something like this:
#
# ./run.sh make xx docker_python2_defconfig defconfig
# ./run.sh make xx menuconfig
# ./run.sh make xx BR2_DEFCONFIG=/root/buildroot/external/configs/docker_python2_defconfig savedefconfig
# ./run.sh make xx
set -e

#Mount volume in docker image
BR_DIR=/home/br/buildroot
BR_DL_DIR=/home/br/buildroot/buildroot/dl
UBOOT_TC_DIR=/opt

#Mount path in local
LOCAL_BR_DL_DIR=${LOCAL_BR_DL_DIR="/mnt/fileroot/buildroot-dl"}
LOCAL_UBOOT_TC_DIR=${LOCAL_UBOOT_TC_DIR="/opt"}

DOCKER_RUN="docker run
    --rm
    -ti
    -v $(pwd):$BR_DIR
    -v $LOCAL_BR_DL_DIR:$BR_DL_DIR
    -v $LOCAL_UBOOT_TC_DIR:$UBOOT_TC_DIR
    aml/buildroot"

if [ "$1" == "make" ]; then
    br_config=$2
    eval $DOCKER_RUN make ${br_config}_defconfig -C buildroot O=$BR_DIR/output/${br_config}
    eval $DOCKER_RUN make -C $BR_DIR/output/${br_config} ${@:3}
else
    eval $DOCKER_RUN $@
fi
