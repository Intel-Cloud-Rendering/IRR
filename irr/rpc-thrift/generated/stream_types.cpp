/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "stream_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace IntelCloudRendering {


StreamInfo::~StreamInfo() throw() {
}


void StreamInfo::__set_url(const std::string& val) {
  this->url = val;
}

void StreamInfo::__set_framerate(const std::string& val) {
  this->framerate = val;
__isset.framerate = true;
}

void StreamInfo::__set_exp_vid_param(const std::string& val) {
  this->exp_vid_param = val;
__isset.exp_vid_param = true;
}

void StreamInfo::__set_vcodec(const std::string& val) {
  this->vcodec = val;
__isset.vcodec = true;
}

void StreamInfo::__set_format(const std::string& val) {
  this->format = val;
__isset.format = true;
}

void StreamInfo::__set_resolution(const std::string& val) {
  this->resolution = val;
__isset.resolution = true;
}
std::ostream& operator<<(std::ostream& out, const StreamInfo& obj)
{
  obj.printTo(out);
  return out;
}


uint32_t StreamInfo::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;

  bool isset_url = false;

  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->url);
          isset_url = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->framerate);
          this->__isset.framerate = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->exp_vid_param);
          this->__isset.exp_vid_param = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->vcodec);
          this->__isset.vcodec = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 5:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->format);
          this->__isset.format = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 6:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->resolution);
          this->__isset.resolution = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  if (!isset_url)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  return xfer;
}

uint32_t StreamInfo::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("StreamInfo");

  xfer += oprot->writeFieldBegin("url", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->url);
  xfer += oprot->writeFieldEnd();

  if (this->__isset.framerate) {
    xfer += oprot->writeFieldBegin("framerate", ::apache::thrift::protocol::T_STRING, 2);
    xfer += oprot->writeString(this->framerate);
    xfer += oprot->writeFieldEnd();
  }
  if (this->__isset.exp_vid_param) {
    xfer += oprot->writeFieldBegin("exp_vid_param", ::apache::thrift::protocol::T_STRING, 3);
    xfer += oprot->writeString(this->exp_vid_param);
    xfer += oprot->writeFieldEnd();
  }
  if (this->__isset.vcodec) {
    xfer += oprot->writeFieldBegin("vcodec", ::apache::thrift::protocol::T_STRING, 4);
    xfer += oprot->writeString(this->vcodec);
    xfer += oprot->writeFieldEnd();
  }
  if (this->__isset.format) {
    xfer += oprot->writeFieldBegin("format", ::apache::thrift::protocol::T_STRING, 5);
    xfer += oprot->writeString(this->format);
    xfer += oprot->writeFieldEnd();
  }
  if (this->__isset.resolution) {
    xfer += oprot->writeFieldBegin("resolution", ::apache::thrift::protocol::T_STRING, 6);
    xfer += oprot->writeString(this->resolution);
    xfer += oprot->writeFieldEnd();
  }
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(StreamInfo &a, StreamInfo &b) {
  using ::std::swap;
  swap(a.url, b.url);
  swap(a.framerate, b.framerate);
  swap(a.exp_vid_param, b.exp_vid_param);
  swap(a.vcodec, b.vcodec);
  swap(a.format, b.format);
  swap(a.resolution, b.resolution);
  swap(a.__isset, b.__isset);
}

StreamInfo::StreamInfo(const StreamInfo& other0) {
  url = other0.url;
  framerate = other0.framerate;
  exp_vid_param = other0.exp_vid_param;
  vcodec = other0.vcodec;
  format = other0.format;
  resolution = other0.resolution;
  __isset = other0.__isset;
}
StreamInfo& StreamInfo::operator=(const StreamInfo& other1) {
  url = other1.url;
  framerate = other1.framerate;
  exp_vid_param = other1.exp_vid_param;
  vcodec = other1.vcodec;
  format = other1.format;
  resolution = other1.resolution;
  __isset = other1.__isset;
  return *this;
}
void StreamInfo::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "StreamInfo(";
  out << "url=" << to_string(url);
  out << ", " << "framerate="; (__isset.framerate ? (out << to_string(framerate)) : (out << "<null>"));
  out << ", " << "exp_vid_param="; (__isset.exp_vid_param ? (out << to_string(exp_vid_param)) : (out << "<null>"));
  out << ", " << "vcodec="; (__isset.vcodec ? (out << to_string(vcodec)) : (out << "<null>"));
  out << ", " << "format="; (__isset.format ? (out << to_string(format)) : (out << "<null>"));
  out << ", " << "resolution="; (__isset.resolution ? (out << to_string(resolution)) : (out << "<null>"));
  out << ")";
}

} // namespace
