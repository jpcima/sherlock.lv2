#!/bin/bash

BASE_NAME="sherlock.lv2"
CI_BUILD_NAME=$1
CI_BUILD_DIR="build-${CI_BUILD_NAME}"

PKG_CONFIG_PATH="/opt/lv2/lib/pkgconfig:/opt/${CI_BUILD_NAME}/lib/pkgconfig:/usr/lib/${CI_BUILD_NAME}/pkgconfig"
TOOLCHAIN_FILE="meson/${CI_BUILD_NAME}"

rm -rf ${CI_BUILD_DIR}
PKG_CONFIG_PATH=${PKG_CONFIG_PATH} meson \
	--prefix="/opt/${CI_BUILD_NAME}" \
	--libdir="lib" \
	--cross-file ${TOOLCHAIN_FILE} ${CI_BUILD_DIR}

ninja -C ${CI_BUILD_DIR}
ninja -C ${CI_BUILD_DIR} install

mkdir -p "${BASE_NAME}-$(cat VERSION)/${CI_BUILD_NAME}/${BASE_NAME}"
cp -r "/opt/${CI_BUILD_NAME}/lib/lv2/${BASE_NAME}/" "${BASE_NAME}-$(cat VERSION)/${CI_BUILD_NAME}/"
