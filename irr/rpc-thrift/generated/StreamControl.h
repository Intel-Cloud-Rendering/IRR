/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef StreamControl_H
#define StreamControl_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include "stream_types.h"

namespace IntelCloudRendering {

#ifdef _MSC_VER
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class StreamControlIf {
 public:
  virtual ~StreamControlIf() {}
  virtual int32_t startStream(const StreamInfo& info) = 0;
  virtual void stopStream() = 0;
  virtual int32_t restartStream(const StreamInfo& info) = 0;
};

class StreamControlIfFactory {
 public:
  typedef StreamControlIf Handler;

  virtual ~StreamControlIfFactory() {}

  virtual StreamControlIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(StreamControlIf* /* handler */) = 0;
};

class StreamControlIfSingletonFactory : virtual public StreamControlIfFactory {
 public:
  StreamControlIfSingletonFactory(const ::apache::thrift::stdcxx::shared_ptr<StreamControlIf>& iface) : iface_(iface) {}
  virtual ~StreamControlIfSingletonFactory() {}

  virtual StreamControlIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(StreamControlIf* /* handler */) {}

 protected:
  ::apache::thrift::stdcxx::shared_ptr<StreamControlIf> iface_;
};

class StreamControlNull : virtual public StreamControlIf {
 public:
  virtual ~StreamControlNull() {}
  int32_t startStream(const StreamInfo& /* info */) {
    int32_t _return = 0;
    return _return;
  }
  void stopStream() {
    return;
  }
  int32_t restartStream(const StreamInfo& /* info */) {
    int32_t _return = 0;
    return _return;
  }
};

typedef struct _StreamControl_startStream_args__isset {
  _StreamControl_startStream_args__isset() : info(false) {}
  bool info :1;
} _StreamControl_startStream_args__isset;

class StreamControl_startStream_args {
 public:

  StreamControl_startStream_args(const StreamControl_startStream_args&);
  StreamControl_startStream_args& operator=(const StreamControl_startStream_args&);
  StreamControl_startStream_args() {
  }

  virtual ~StreamControl_startStream_args() throw();
  StreamInfo info;

  _StreamControl_startStream_args__isset __isset;

  void __set_info(const StreamInfo& val);

  bool operator == (const StreamControl_startStream_args & rhs) const
  {
    if (!(info == rhs.info))
      return false;
    return true;
  }
  bool operator != (const StreamControl_startStream_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const StreamControl_startStream_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class StreamControl_startStream_pargs {
 public:


  virtual ~StreamControl_startStream_pargs() throw();
  const StreamInfo* info;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _StreamControl_startStream_result__isset {
  _StreamControl_startStream_result__isset() : success(false) {}
  bool success :1;
} _StreamControl_startStream_result__isset;

class StreamControl_startStream_result {
 public:

  StreamControl_startStream_result(const StreamControl_startStream_result&);
  StreamControl_startStream_result& operator=(const StreamControl_startStream_result&);
  StreamControl_startStream_result() : success(0) {
  }

  virtual ~StreamControl_startStream_result() throw();
  int32_t success;

  _StreamControl_startStream_result__isset __isset;

  void __set_success(const int32_t val);

  bool operator == (const StreamControl_startStream_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const StreamControl_startStream_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const StreamControl_startStream_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _StreamControl_startStream_presult__isset {
  _StreamControl_startStream_presult__isset() : success(false) {}
  bool success :1;
} _StreamControl_startStream_presult__isset;

class StreamControl_startStream_presult {
 public:


  virtual ~StreamControl_startStream_presult() throw();
  int32_t* success;

  _StreamControl_startStream_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};


class StreamControl_stopStream_args {
 public:

  StreamControl_stopStream_args(const StreamControl_stopStream_args&);
  StreamControl_stopStream_args& operator=(const StreamControl_stopStream_args&);
  StreamControl_stopStream_args() {
  }

  virtual ~StreamControl_stopStream_args() throw();

  bool operator == (const StreamControl_stopStream_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const StreamControl_stopStream_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const StreamControl_stopStream_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class StreamControl_stopStream_pargs {
 public:


  virtual ~StreamControl_stopStream_pargs() throw();

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class StreamControl_stopStream_result {
 public:

  StreamControl_stopStream_result(const StreamControl_stopStream_result&);
  StreamControl_stopStream_result& operator=(const StreamControl_stopStream_result&);
  StreamControl_stopStream_result() {
  }

  virtual ~StreamControl_stopStream_result() throw();

  bool operator == (const StreamControl_stopStream_result & /* rhs */) const
  {
    return true;
  }
  bool operator != (const StreamControl_stopStream_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const StreamControl_stopStream_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class StreamControl_stopStream_presult {
 public:


  virtual ~StreamControl_stopStream_presult() throw();

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _StreamControl_restartStream_args__isset {
  _StreamControl_restartStream_args__isset() : info(false) {}
  bool info :1;
} _StreamControl_restartStream_args__isset;

class StreamControl_restartStream_args {
 public:

  StreamControl_restartStream_args(const StreamControl_restartStream_args&);
  StreamControl_restartStream_args& operator=(const StreamControl_restartStream_args&);
  StreamControl_restartStream_args() {
  }

  virtual ~StreamControl_restartStream_args() throw();
  StreamInfo info;

  _StreamControl_restartStream_args__isset __isset;

  void __set_info(const StreamInfo& val);

  bool operator == (const StreamControl_restartStream_args & rhs) const
  {
    if (!(info == rhs.info))
      return false;
    return true;
  }
  bool operator != (const StreamControl_restartStream_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const StreamControl_restartStream_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class StreamControl_restartStream_pargs {
 public:


  virtual ~StreamControl_restartStream_pargs() throw();
  const StreamInfo* info;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _StreamControl_restartStream_result__isset {
  _StreamControl_restartStream_result__isset() : success(false) {}
  bool success :1;
} _StreamControl_restartStream_result__isset;

class StreamControl_restartStream_result {
 public:

  StreamControl_restartStream_result(const StreamControl_restartStream_result&);
  StreamControl_restartStream_result& operator=(const StreamControl_restartStream_result&);
  StreamControl_restartStream_result() : success(0) {
  }

  virtual ~StreamControl_restartStream_result() throw();
  int32_t success;

  _StreamControl_restartStream_result__isset __isset;

  void __set_success(const int32_t val);

  bool operator == (const StreamControl_restartStream_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const StreamControl_restartStream_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const StreamControl_restartStream_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _StreamControl_restartStream_presult__isset {
  _StreamControl_restartStream_presult__isset() : success(false) {}
  bool success :1;
} _StreamControl_restartStream_presult__isset;

class StreamControl_restartStream_presult {
 public:


  virtual ~StreamControl_restartStream_presult() throw();
  int32_t* success;

  _StreamControl_restartStream_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class StreamControlClient : virtual public StreamControlIf {
 public:
  StreamControlClient(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  StreamControlClient(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  int32_t startStream(const StreamInfo& info);
  void send_startStream(const StreamInfo& info);
  int32_t recv_startStream();
  void stopStream();
  void send_stopStream();
  void recv_stopStream();
  int32_t restartStream(const StreamInfo& info);
  void send_restartStream(const StreamInfo& info);
  int32_t recv_restartStream();
 protected:
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class StreamControlProcessor : public ::apache::thrift::TDispatchProcessor {
 protected:
  ::apache::thrift::stdcxx::shared_ptr<StreamControlIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (StreamControlProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_startStream(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_stopStream(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_restartStream(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  StreamControlProcessor(::apache::thrift::stdcxx::shared_ptr<StreamControlIf> iface) :
    iface_(iface) {
    processMap_["startStream"] = &StreamControlProcessor::process_startStream;
    processMap_["stopStream"] = &StreamControlProcessor::process_stopStream;
    processMap_["restartStream"] = &StreamControlProcessor::process_restartStream;
  }

  virtual ~StreamControlProcessor() {}
};

class StreamControlProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  StreamControlProcessorFactory(const ::apache::thrift::stdcxx::shared_ptr< StreamControlIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::apache::thrift::stdcxx::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::apache::thrift::stdcxx::shared_ptr< StreamControlIfFactory > handlerFactory_;
};

class StreamControlMultiface : virtual public StreamControlIf {
 public:
  StreamControlMultiface(std::vector<apache::thrift::stdcxx::shared_ptr<StreamControlIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~StreamControlMultiface() {}
 protected:
  std::vector<apache::thrift::stdcxx::shared_ptr<StreamControlIf> > ifaces_;
  StreamControlMultiface() {}
  void add(::apache::thrift::stdcxx::shared_ptr<StreamControlIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  int32_t startStream(const StreamInfo& info) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->startStream(info);
    }
    return ifaces_[i]->startStream(info);
  }

  void stopStream() {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->stopStream();
    }
    ifaces_[i]->stopStream();
  }

  int32_t restartStream(const StreamInfo& info) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->restartStream(info);
    }
    return ifaces_[i]->restartStream(info);
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class StreamControlConcurrentClient : virtual public StreamControlIf {
 public:
  StreamControlConcurrentClient(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  StreamControlConcurrentClient(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  int32_t startStream(const StreamInfo& info);
  int32_t send_startStream(const StreamInfo& info);
  int32_t recv_startStream(const int32_t seqid);
  void stopStream();
  int32_t send_stopStream();
  void recv_stopStream(const int32_t seqid);
  int32_t restartStream(const StreamInfo& info);
  int32_t send_restartStream(const StreamInfo& info);
  int32_t recv_restartStream(const int32_t seqid);
 protected:
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
  ::apache::thrift::async::TConcurrentClientSyncInfo sync_;
};

#ifdef _MSC_VER
  #pragma warning( pop )
#endif

} // namespace

#endif
