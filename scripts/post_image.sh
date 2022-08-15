#!/usr/bin/env bash
#
# script to prepare disk images from project

PRODUCT_PATH=$1
IMAGE_PATH=$2
IMAGE_NAME=$3

echo 'imaging disk ['${IMAGE_NAME}'] from '${PRODUCT_PATH}

# put your disk image operations here!

#mkdir -p ${IMAGE_PATH}
ls -l ${PRODUCT_PATH}
