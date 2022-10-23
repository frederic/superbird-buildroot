#docker build -t aml/buildroot .
#Provide the current user's uid(955) and groupid(502)
FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

#Uboot require external toolchain
ENV PATH=${PATH}:/opt/CodeSourcery/Sourcery_G++_Lite/bin/:/opt/gcc-linaro-aarch64-none-elf-4.9-2014.09_linux/bin/:/opt/gcc-linaro-7.3.1-2018.05-i686_aarch64-elf/bin/

#Amlogic internal use for DSP RTOS
ENV PATH=${PATH}:/opt/xtensa/XtDevTools/install/tools/RG-2018.9-linux/XtensaTools/bin/
ENV LM_LICENSE_FILE="7788@10.18.11.22:5280@10.18.11.22:27000@10.18.11.22:27002@10.18.30.103:27002@10.18.30.122:27000@10.18.30.254"
ENV XTENSA_SYSTEM=/opt/xtensa/XtDevTools/install/builds/RG-2018.9-linux/tm2_v5_prod/config

RUN dpkg --add-architecture i386
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    bash \
    bc \
    binutils \
    build-essential \
    bzip2 \
    cpio \
    g++ \
    gcc \
    git \
    gzip \
    locales \
    libncurses5-dev \
    libdevmapper-dev \
    libsystemd-dev \
    make \
    mercurial \
    whois \
    patch \
    perl \
    python \
    rsync \
    sed \
    tar \
    vim \
    unzip \
    wget \
    bison \
    flex \
    libssl-dev \
    libfdt-dev \
    libc6:i386 libncurses5:i386 libstdc++6:i386 \
    zlib1g-dev:i386 \
    zip \
    python-pip \
    pkg-config \
    automake \
    gsettings-ubuntu-schemas \
    libglib2.0-dev \
    gcc-multilib g++-multilib

#uboot toolchain require 32bit binary and library support
#TDK require pycrypto
RUN pip install pycrypto

RUN wget http://ftp.cn.debian.org/debian/pool/main/a/automake-1.16/automake_1.16.1-4_all.deb && dpkg -i automake_1.16.1-4_all.deb && rm -f automake_1.16.1-4_all.deb

# Sometimes Buildroot need proper locale, e.g. when using a toolchain
# based on glibc.
RUN locale-gen en_US.utf8 && update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

RUN mkdir -p /home/br && chmod a+w /home/br
RUN groupadd -g 502 brgroup && useradd -r -g 502 -u 955 br

USER br
WORKDIR /home/br/buildroot

VOLUME /opt
VOLUME /home/br/buildroot
VOLUME /home/br/buildroot/buildroot/dl

RUN ["/bin/bash"]
