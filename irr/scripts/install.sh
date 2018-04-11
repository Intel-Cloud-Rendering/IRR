#!/bin/bash

# Copyright 2018 Intel Corporation All Rights Reserved.
#
# The source code, information and material ("Material") contained
# herein is owned by Intel Corporation or its suppliers or licensors,
# and title to such Material remains with Intel Corporation or its
# suppliers or licensors. The Material contains proprietary information
# of Intel or its suppliers and licensors. The Material is protected by
# worldwide copyright laws and treaty provisions. No part of the
# Material may be used, copied, reproduced, modified, published,
# uploaded, posted, transmitted, distributed or disclosed in any way
# without Intel's prior express written permission. No license under any
# patent, copyright or other intellectual property rights in the
# Material is granted to or conferred upon you, either expressly, by
# implication, inducement, estoppel or otherwise. Any license under such
# intellectual property rights must be express and approved by Intel in
# writing.
#
# Unless otherwise agreed by Intel in writing, you may not remove or alter
# this notice or any other notice embedded in Materials by Intel or Intel's
# suppliers or licensors in any way.


## Format: sudo ./install.sh

FROM_PATH=.
TO_PATH=/opt/intel/cloudrendering

# Try command  for test command result.
function try_command {
    "$@"
    status=$?
    if [ $status -ne 0 ]; then
        echo -e $ECHO_PREFIX_ERROR "ERROR with \"$@\", Return status $status."
        exit $status
    fi
    return $status
}


# This script must be run as root
if [[ $EUID -ne 0 ]]; then
    echo -e $ECHO_PREFIX_ERROR "This script must be run as root!" 1>&2
    exit 1
fi

## install pre-requisite package
try_command apt-get install mesa-utils xorg libgl1-mesa-glx libgl1-mesa-dev

## remove old installation
try_command rm -fr $TO_PATH

## install files
## make target install folder
if [ ! -d "$TO_PATH" ]; then
    try_command mkdir -p "$TO_PATH"
fi
if [ ! -d "$TO_PATH/bin" ]; then
    try_command mkdir -p "$TO_PATH/bin"
fi
if [ ! -d "$TO_PATH/lib64" ]; then
    try_command mkdir -p "$TO_PATH/lib64"
fi

## copy executable and library
try_command cp $FROM_PATH/bin/* $TO_PATH/bin/
try_command cp $FROM_PATH/lib64/* $TO_PATH/lib64/

echo "Install Successfully!"


