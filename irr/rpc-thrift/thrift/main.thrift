# IntelCloudRendering RPC definition
#
# This part is based on thrift-0.11.0 and aims to process all
# inter-process commands.

/**
 * The pre-definitions, such as base classes, namespaces and so on.
 */
include "stream.thrift"

namespace cpp IntelCloudRendering

/**
 * Use "extends filename.sevice" to add customized rpc command to
 * main services.
 */
service IrrControl extends stream.StreamControl {
    /* To check if server is online */
    void ping();
}

