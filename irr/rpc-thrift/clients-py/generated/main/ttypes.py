#
# Autogenerated by Thrift Compiler (0.11.0)
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#
#  options string: py
#

from thrift.Thrift import TType, TMessageType, TFrozenDict, TException, TApplicationException
from thrift.protocol.TProtocol import TProtocolException
from thrift.TRecursive import fix_spec

import sys
import stream.ttypes

from thrift.transport import TTransport
all_structs = []


class DumpInfo(object):
    """
    Attributes:
     - filename
     - serial_no
     - frame_total
     - dur_s
    """


    def __init__(self, filename=None, serial_no=None, frame_total=None, dur_s=None,):
        self.filename = filename
        self.serial_no = serial_no
        self.frame_total = frame_total
        self.dur_s = dur_s

    def read(self, iprot):
        if iprot._fast_decode is not None and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None:
            iprot._fast_decode(self, iprot, [self.__class__, self.thrift_spec])
            return
        iprot.readStructBegin()
        while True:
            (fname, ftype, fid) = iprot.readFieldBegin()
            if ftype == TType.STOP:
                break
            if fid == 1:
                if ftype == TType.STRING:
                    self.filename = iprot.readString().decode('utf-8') if sys.version_info[0] == 2 else iprot.readString()
                else:
                    iprot.skip(ftype)
            elif fid == 2:
                if ftype == TType.I32:
                    self.serial_no = iprot.readI32()
                else:
                    iprot.skip(ftype)
            elif fid == 3:
                if ftype == TType.I32:
                    self.frame_total = iprot.readI32()
                else:
                    iprot.skip(ftype)
            elif fid == 4:
                if ftype == TType.I32:
                    self.dur_s = iprot.readI32()
                else:
                    iprot.skip(ftype)
            else:
                iprot.skip(ftype)
            iprot.readFieldEnd()
        iprot.readStructEnd()

    def write(self, oprot):
        if oprot._fast_encode is not None and self.thrift_spec is not None:
            oprot.trans.write(oprot._fast_encode(self, [self.__class__, self.thrift_spec]))
            return
        oprot.writeStructBegin('DumpInfo')
        if self.filename is not None:
            oprot.writeFieldBegin('filename', TType.STRING, 1)
            oprot.writeString(self.filename.encode('utf-8') if sys.version_info[0] == 2 else self.filename)
            oprot.writeFieldEnd()
        if self.serial_no is not None:
            oprot.writeFieldBegin('serial_no', TType.I32, 2)
            oprot.writeI32(self.serial_no)
            oprot.writeFieldEnd()
        if self.frame_total is not None:
            oprot.writeFieldBegin('frame_total', TType.I32, 3)
            oprot.writeI32(self.frame_total)
            oprot.writeFieldEnd()
        if self.dur_s is not None:
            oprot.writeFieldBegin('dur_s', TType.I32, 4)
            oprot.writeI32(self.dur_s)
            oprot.writeFieldEnd()
        oprot.writeFieldStop()
        oprot.writeStructEnd()

    def validate(self):
        if self.filename is None:
            raise TProtocolException(message='Required field filename is unset!')
        if self.serial_no is None:
            raise TProtocolException(message='Required field serial_no is unset!')
        return

    def __repr__(self):
        L = ['%s=%r' % (key, value)
             for key, value in self.__dict__.items()]
        return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

    def __ne__(self, other):
        return not (self == other)
all_structs.append(DumpInfo)
DumpInfo.thrift_spec = (
    None,  # 0
    (1, TType.STRING, 'filename', 'UTF8', None, ),  # 1
    (2, TType.I32, 'serial_no', None, None, ),  # 2
    (3, TType.I32, 'frame_total', None, None, ),  # 3
    (4, TType.I32, 'dur_s', None, None, ),  # 4
)
fix_spec(all_structs)
del all_structs