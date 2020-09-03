#!/bin/bash -ex
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

source tests/ci/common_posix_setup.sh

AWS_LC_DIR=${PWD##*/}
cd ../
ROOT=$(pwd)
mkdir -p aws-lc-build aws-lc-install s2n-build
git clone https://github.com/awslabs/s2n.git
ls

# s2n's FindLibCrypto.cmake expects to find both the archieve (.a) and shared object (.so) libcrypto
cmake ${AWS_LC_DIR} -GNinja "-B${ROOT}/aws-lc-build" "-DCMAKE_INSTALL_PREFIX=${ROOT}/aws-lc-install"
ninja -C aws-lc-build install
rm -rf aws-lc-build/*
cmake ${AWS_LC_DIR} -GNinja "-B${ROOT}/aws-lc-build" "-DCMAKE_INSTALL_PREFIX=${ROOT}/aws-lc-install" -DBUILD_SHARED_LIBS=1
ninja -C aws-lc-build install

# While AWS-LC is a private project pretend to be BoringSSL so s2n uses the correct APIs
# TODO: delete below CFLAGS when S2N merges PR/2273.
CFLAGS="-DOPENSSL_IS_BORINGSSL=1"
export CFLAGS

cmake s2n -GNinja "-B${ROOT}/s2n-build" "-DCMAKE_PREFIX_PATH=${ROOT}/aws-lc-install"
ninja -C s2n-build

cd "${ROOT}/s2n-build"
ctest -j 8 --output-on-failure
