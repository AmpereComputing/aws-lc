#!/bin/bash -ex
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

if [ -z ${1+x} ]; then
  ECS_REPO="620771051181.dkr.ecr.us-west-2.amazonaws.com/linux-arm-docker-images"
else
  ECS_REPO=$1
fi

echo "Uploading docker images to ${ECS_REPO}."

$(aws ecr get-login --no-include-email --region us-west-2)

# Tag images with date to help find old images, CodeBuild uses the latest tag and gets updated automatically
docker tag ubuntu-19.10-aarch:gcc-9x ${ECS_REPO}:ubuntu-19.10_gcc-9x_`date +%Y-%m-%d`
docker tag ubuntu-19.10-aarch:gcc-9x ${ECS_REPO}:ubuntu-19.10_gcc-9x_latest
docker push ${ECS_REPO}:ubuntu-19.10_gcc-9x_latest
docker push ${ECS_REPO}:ubuntu-19.10_gcc-9x_`date +%Y-%m-%d`

docker tag ubuntu-19.10-aarch:clang-9x ${ECS_REPO}:ubuntu-19.10_clang-9x_`date +%Y-%m-%d`
docker tag ubuntu-19.10-aarch:clang-9x ${ECS_REPO}:ubuntu-19.10_clang-9x_latest
docker push ${ECS_REPO}:ubuntu-19.10_clang-9x_latest
docker push ${ECS_REPO}:ubuntu-19.10_clang-9x_`date +%Y-%m-%d`

docker tag ubuntu-19.10-aarch:sanitizer ${ECS_REPO}:ubuntu-19.10_clang-9x_sanitizer_`date +%Y-%m-%d`
docker tag ubuntu-19.10-aarch:sanitizer ${ECS_REPO}:ubuntu-19.10_clang-9x_sanitizer_latest
docker push ${ECS_REPO}:ubuntu-19.10_clang-9x_sanitizer_latest
docker push ${ECS_REPO}:ubuntu-19.10_clang-9x_sanitizer_`date +%Y-%m-%d`
