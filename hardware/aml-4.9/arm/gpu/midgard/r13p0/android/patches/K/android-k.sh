#!/bin/bash

#
# Copyright (C) 2014-2015 ARM Limited. All rights reserved.
#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Android K script for LLVM 3.5 and above support
#
# This script applies two minor patches and checks-out libcxx and libcxxabi libraries
# adding C++11 support in Android K required for LLVM3.5 and above.


if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "[ANDROID-PATCH] This script must be executed in an Android build environment"
    echo "                ANDROID_BUILD_TOP is undefined"
    echo "                please do \"source build/envseup.sh; lunch\"."
    exit 1
fi

ANDROID_REPOSITORY_URL="https://android.googlesource.com/"

REFERENCE_ARGUMENT=""

# Proccess arguments. Valid arguments are:
#        -u ARG      Override the Android repository url
#        -r ARG      Provide a path to a reference git repository
while getopts "u:r:" opt; do
  case $opt in
    u)
      ANDROID_REPOSITORY_URL=$OPTARG
      ;;

    r)
      REFERENCE_ARGUMENT="--reference "$OPTARG
      ;;

    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1

      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;

  esac
done

MIDGARD_DDK_WD=$(pwd)

cd $ANDROID_BUILD_TOP/external

#Checkout libcxx from Android Master required by LLVM due to c++11
git clone $ANDROID_REPOSITORY_URL/platform/external/libcxx $REFERENCE_ARGUMENT

ret_code=$?
if [[ $ret_code != 0 ]] ; then
    echo "[ANDROID-PATCH] git clone of libcxx failed."
    cd $MIDGARD_DDK_WD
    exit $ret_code
fi

cd libcxx
git checkout 27ae7cb782821a4f2d3813522ee411cd978bcd85 -q

ret_code=$?
if [[ $ret_code != 0 ]] ; then
    echo "[ANDROID-PATCH] checkout of libcxx@27ae7cb782821a4f2d3813522ee411cd978bcd85 revision failed."
    cd $MIDGARD_DDK_WD
    exit $ret_code
fi

patch -p1  < $MIDGARD_DDK_WD/android/patches/K/libcxx.diff

ret_code=$?
if [[ $ret_code != 0 ]] ; then
    echo "[ANDROID-PATCH] Patching libcxx failed."
    git reset --hard
    cd $MIDGARD_DDK_WD
    exit $ret_code
fi

#Checkout libcxxabi from Android Master required by LLVM due to c++11
cd $ANDROID_BUILD_TOP/external
git clone $ANDROID_REPOSITORY_URL/platform/external/libcxxabi $REFERENCE_ARGUMENT

ret_code=$?
if [[ $ret_code != 0 ]] ; then
    echo "[ANDROID-PATCH] git clone of libcxxabi failed."
    cd $MIDGARD_DDK_WD
    exit $ret_code
fi

cd libcxxabi
git checkout 285d67f35f6044cf733091e36248405ca967c62c -q

ret_code=$?
if [[ $ret_code != 0 ]] ; then
    echo "[ANDROID-PATCH] checkout of libcxxabi@285d67f35f6044cf733091e36248405ca967c62c revision failed."
    cd $MIDGARD_DDK_WD
    exit $ret_code
fi


#Apply patch to bioncs
cd $ANDROID_BUILD_TOP/bionic/
patch -p1  < $MIDGARD_DDK_WD/android/patches/K/bionics.diff

ret_code=$?
if [[ $ret_code != 0 ]] ; then
    echo "[ANDROID-PATCH] Patching bionics failed."
    git reset --hard
    cd $MIDGARD_DDK_WD
    exit $ret_code
fi

#Add locale support to bioncs
cd $ANDROID_BUILD_TOP/bionic/
patch -p1  < $MIDGARD_DDK_WD/android/patches/K/locale.diff

ret_code=$?
if [[ $ret_code != 0 ]] ; then
    echo "[ANDROID-PATCH] Adding locale support failed."
    git reset --hard
    cd $MIDGARD_DDK_WD
    exit $ret_code
fi

#Add LLVM backend ARM fix to support -O0 compilation
cd $ANDROID_BUILD_TOP/external/llvm/
patch -p1  < $MIDGARD_DDK_WD/android/patches/K/llvm.diff

ret_code=$?
if [[ $ret_code != 0 ]] ; then
    echo "[ANDROID-PATCH] Fixing ARM LLVM backend failed."
    git reset --hard
    cd $MIDGARD_DDK_WD
    exit $ret_code
fi

echo "[ANDROID-PATCH] Patching completed successfully"
cd $MIDGARD_DDK_WD
