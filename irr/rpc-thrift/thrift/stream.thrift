# IntelCloudRendering RPC definition
# ChaoX A Liu (chaox.a.liu@intel.com)
#
# This part is based on thrift-0.11.0 and aims to process all
# streaming related commands.

/**
 * The pre-definitions, such as base classes, namespaces and so on.
 */
namespace cpp IntelCloudRendering

struct StreamInfo {
    1: required string url,            // Destination url
    2: optional string framerate,      // Encoding framerate
    3: optional string exp_vid_param,  // Advanced "=:" separated params, i.e "bf=2:g=25"
    4: optional string vcodec,         // Encoding video codec, i.e h264_vaapi
    5: optional string format,         // File format, i.e flv mpegts
    6: optional string resolution,     // Encoding resolution
}

service StreamControl {
    // Start a new stream, fails if one is already running
    i32 startStream(1: StreamInfo info),
    // Stop a running streaming
    void stopStream(),
    // restart, a combination of stop and start
    i32 restartStream(1: StreamInfo info)
}

