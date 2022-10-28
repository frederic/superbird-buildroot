# [superbird-buildroot](https://github.com/frederic/superbird-buildroot)

Since Spotify Car Thing (superbird) [has been unlocked](https://github.com/frederic/superbird-bulkcmd), the device can now run custom software.
This repository provides a [Buildroot](https://buildroot.org/) environment to build software for superbird target.

# requirements
- Debian-based Linux host
- Docker

# setup
```shell
sudo apt install bison
git clone https://github.com/frederic/superbird-buildroot.git
cd superbird-buildroot/
git checkout buildroot-openlinux
wget https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/aarch64-linux-gnu/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz -O vendor/amlogic/external/packages/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz
wget https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/arm-linux-gnueabihf/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz -O vendor/amlogic/external/packages/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz
git clone https://cobalt.googlesource.com/cobalt
cd cobalt
git archive --format=tar.gz -o ../vendor/amlogic/external/packages/cobalt-20.lts.3.244012.tar.gz -v 20.lts.3
cd ..
```

# run
(Based on https://wiki.odroid.com/odroid-n2/software/using_docker)
```shell
cd superbird-buildroot/
docker run --name superbird_buildroot -v $PWD:/srv -it odroid/meson64-dev:202106
```

## resume stopped container
```shell
docker start -i superbird_buildroot
```

# build
In docker :
```shell
. ./setenv.sh mesong12a_u200_32_release
make
```
To build a specific package :
```shell
cd buildroot/
make mesong12a_u200_32_release_defconfig
make weston
```