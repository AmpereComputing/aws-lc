#!/usr/bin/env bash
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0 OR ISC

set -exu

source tests/ci/common_posix_setup.sh

# Set up environment.

# SYS_ROOT
#  - SRC_ROOT(aws-lc)
#    - SCRATCH_FOLDER
#      - OPENVPN_SRC_FOLDER
#      - AWS_LC_BUILD_FOLDER
#      - AWS_LC_INSTALL_FOLDER

# Assumes script is executed from the root of aws-lc directory
SCRATCH_FOLDER="${SRC_ROOT}/NMAP_BUILD_ROOT"
NMAP_SRC_FOLDER="${SCRATCH_FOLDER}/nmap"
NMAP_BUILD_PREFIX="${NMAP_SRC_FOLDER}/build/install"
NMAP_BUILD_EPREFIX="${NMAP_SRC_FOLDER}/build/exec-install"
NMAP_PATCH_BUILD_FOLDER="${SRC_ROOT}/tests/ci/integration/nmap_patch"

AWS_LC_BUILD_FOLDER="${SCRATCH_FOLDER}/aws-lc-build"
AWS_LC_INSTALL_FOLDER="${SCRATCH_FOLDER}/aws-lc-install"

mkdir -p ${SCRATCH_FOLDER}
rm -rf "${SCRATCH_FOLDER:?}"/*
cd ${SCRATCH_FOLDER}

function nmap_build() {

  OPENSSL_CFLAGS="-I/${AWS_LC_INSTALL_FOLDER}/include" \
  OPENSSL_LIBS="-L/${AWS_LC_INSTALL_FOLDER}/lib -lssl -lcrypto" \
  ./configure \
    --prefix="$OPENVPN_BUILD_PREFIX" \
    --exec-prefix="$OPENVPN_BUILD_EPREFIX" \
    --with-crypto-library=openssl \
    --with-openssl-engine=no

  make -j install

  export LD_LIBRARY_PATH="${AWS_LC_INSTALL_FOLDER}/lib"

#  local openvpn_executable="${OPENVPN_SRC_FOLDER}/build/exec-install/sbin/openvpn"
#  ldd ${openvpn_executable} \
#    | grep "${AWS_LC_INSTALL_FOLDER}/lib/libcrypto.so" || exit 1
}

# TODO: Remove this when we make an upstream contribution.
function openvpn_patch_build() {
  case "$BRANCH_NAME" in
    "release/2.6")
      patchfile="${OPENVPN_PATCH_BUILD_FOLDER}/aws-lc-openvpn2-6-x.patch"
      ;;
    "master")
      patchfile="${OPENVPN_PATCH_BUILD_FOLDER}/aws-lc-openvpn-master.patch"
      ;;
    *)
      echo "No specific patch file for branch: $BRANCH_NAME"
      exit 1
      ;;
  esac

  echo "Apply patch $patchfile..."
  patch -p1 --quiet -i "$patchfile"
}

function openvpn_run_tests() {
  # Explicitly running as sudo and passing in LD_LIBRARY_PATH as some OpenVPN
  # tests run as sudo and LD_LIBRARY_PATH doesn't get inherited.
  sudo LD_LIBRARY_PATH="${AWS_LC_INSTALL_FOLDER}/lib" make check
}

git clone https://github.com/OpenVPN/openvpn.git ${OPENVPN_SRC_FOLDER}
cd ${OPENVPN_SRC_FOLDER} && git checkout $BRANCH_NAME
mkdir -p ${AWS_LC_BUILD_FOLDER} ${AWS_LC_INSTALL_FOLDER}
ls

aws_lc_build "$SRC_ROOT" "$AWS_LC_BUILD_FOLDER" "$AWS_LC_INSTALL_FOLDER" -DBUILD_TESTING=OFF -DBUILD_TOOL=OFF -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=1

# Build openvpn from source.
pushd ${OPENVPN_SRC_FOLDER}
openvpn_patch_build
openvpn_build
openvpn_run_tests
