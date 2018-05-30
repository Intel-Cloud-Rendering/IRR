#!/bin/bash

mkdir -p ./generated
../../../rrndr-prebuilts/thrift/linux-x86_64/bin/thrift -gen py -r -out ./generated ../../rpc-thrift/thrift/main.thrift
../../../rrndr-prebuilts/thrift/linux-x86_64/bin/thrift -gen py -r -out ./generated ../../rpc-thrift/thrift/stream.thrift

mv generated/main/IrrControl-remote generated/

