/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef stream_TYPES_H
#define stream_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/TBase.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/stdcxx.h>


namespace IntelCloudRendering {

class StreamInfo;

typedef struct _StreamInfo__isset {
  _StreamInfo__isset() : framerate(false), exp_vid_param(false), vcodec(false), format(false), resolution(false) {}
  bool framerate :1;
  bool exp_vid_param :1;
  bool vcodec :1;
  bool format :1;
  bool resolution :1;
} _StreamInfo__isset;

class StreamInfo : public virtual ::apache::thrift::TBase {
 public:

  StreamInfo(const StreamInfo&);
  StreamInfo& operator=(const StreamInfo&);
  StreamInfo() : url(), framerate(), exp_vid_param(), vcodec(), format(), resolution() {
  }

  virtual ~StreamInfo() throw();
  std::string url;
  std::string framerate;
  std::string exp_vid_param;
  std::string vcodec;
  std::string format;
  std::string resolution;

  _StreamInfo__isset __isset;

  void __set_url(const std::string& val);

  void __set_framerate(const std::string& val);

  void __set_exp_vid_param(const std::string& val);

  void __set_vcodec(const std::string& val);

  void __set_format(const std::string& val);

  void __set_resolution(const std::string& val);

  bool operator == (const StreamInfo & rhs) const
  {
    if (!(url == rhs.url))
      return false;
    if (__isset.framerate != rhs.__isset.framerate)
      return false;
    else if (__isset.framerate && !(framerate == rhs.framerate))
      return false;
    if (__isset.exp_vid_param != rhs.__isset.exp_vid_param)
      return false;
    else if (__isset.exp_vid_param && !(exp_vid_param == rhs.exp_vid_param))
      return false;
    if (__isset.vcodec != rhs.__isset.vcodec)
      return false;
    else if (__isset.vcodec && !(vcodec == rhs.vcodec))
      return false;
    if (__isset.format != rhs.__isset.format)
      return false;
    else if (__isset.format && !(format == rhs.format))
      return false;
    if (__isset.resolution != rhs.__isset.resolution)
      return false;
    else if (__isset.resolution && !(resolution == rhs.resolution))
      return false;
    return true;
  }
  bool operator != (const StreamInfo &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const StreamInfo & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(StreamInfo &a, StreamInfo &b);

std::ostream& operator<<(std::ostream& out, const StreamInfo& obj);

} // namespace

#endif
