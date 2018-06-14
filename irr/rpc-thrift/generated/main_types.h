/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef main_TYPES_H
#define main_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/TBase.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/stdcxx.h>
#include "stream_types.h"


namespace IntelCloudRendering {

class DumpInfo;

typedef struct _DumpInfo__isset {
  _DumpInfo__isset() : frame_total(false), dur_s(false) {}
  bool frame_total :1;
  bool dur_s :1;
} _DumpInfo__isset;

class DumpInfo : public virtual ::apache::thrift::TBase {
 public:

  DumpInfo(const DumpInfo&);
  DumpInfo& operator=(const DumpInfo&);
  DumpInfo() : filename(), serial_no(0), frame_total(0), dur_s(0) {
  }

  virtual ~DumpInfo() throw();
  std::string filename;
  int32_t serial_no;
  int32_t frame_total;
  int32_t dur_s;

  _DumpInfo__isset __isset;

  void __set_filename(const std::string& val);

  void __set_serial_no(const int32_t val);

  void __set_frame_total(const int32_t val);

  void __set_dur_s(const int32_t val);

  bool operator == (const DumpInfo & rhs) const
  {
    if (!(filename == rhs.filename))
      return false;
    if (!(serial_no == rhs.serial_no))
      return false;
    if (__isset.frame_total != rhs.__isset.frame_total)
      return false;
    else if (__isset.frame_total && !(frame_total == rhs.frame_total))
      return false;
    if (__isset.dur_s != rhs.__isset.dur_s)
      return false;
    else if (__isset.dur_s && !(dur_s == rhs.dur_s))
      return false;
    return true;
  }
  bool operator != (const DumpInfo &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const DumpInfo & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(DumpInfo &a, DumpInfo &b);

std::ostream& operator<<(std::ostream& out, const DumpInfo& obj);

} // namespace

#endif