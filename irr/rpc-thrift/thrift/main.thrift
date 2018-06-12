# IntelCloudRendering RPC definition
#
# This part is based on thrift-0.11.0 and aims to process all
# inter-process commands.

/**
 * The pre-definitions, such as base classes, namespaces and so on.
 */
include "stream.thrift"

namespace cpp IntelCloudRendering

struct DumpInfo {
    1: required string filename,    // Dump file name
    2: required i32 serial_no,      // Dump file serial NO.
    3: optional i32 frame_total,    // Dump frame count
    4: optional i32 dur_s,    // Dump timeout set
}

/**
 * Use "extends filename.sevice" to add customized rpc command to
 * main services.
 */
service IrrControl extends stream.StreamControl {
    /* To check if server is online */
    void ping();
    /* Start dump */
    i32 startDump(1: DumpInfo info);
    /* Stop dump */
    i32 stopDump();
    /* Restart dump */
    i32 restartDump(1: DumpInfo info);
    /* return dump status */
    bool readDumpStatus();

    /* force key frame encoding */
    i32 forceKeyFrame(1:i32 force_key_frame);
}

