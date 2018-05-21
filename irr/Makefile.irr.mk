LOCAL_PATH := $(call my-dir)
ROOT_PATH :=/home/jhou15/workspace/src/ssg/android-sc/external/qemu-irr

QEMU2_TARGET := x86_64
QEMU2_TARGET_TARGET := i386

# Build intel remote renderer
$(call start-emulator-program, intel_remote_renderer)

#LOCAL_WHOLE_STATIC_LIBRARIES += \
    libqemu2-system-x86_64 \
	libqemu2-common \


LOCAL_STATIC_LIBRARIES += \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    $(QEMU2_GLUE_STATIC_LIBRARIES) \
#	$(QEMU2_SYSTEM_STATIC_LIBRARIES) \

LOCAL_CFLAGS += \
    $(QEMU2_SYSTEM_CFLAGS) \
    $(THRIFT_CFLAGS) \
    -DCONFIG_IRR \
    -fexceptions \

LOCAL_C_INCLUDES += \
    $(ANDROID_EMU_INCLUDES) \
    $(EMUGL_INCLUDES) \
    $(QEMU2_INCLUDES) \
    $(THRIFT_INCLUDES) \
#   $(QEMU2_DEPS_TOP_DIR)/include \
    $(call qemu2-if-linux,$(ROOT_PATH)/linux-headers) \
    $(ROOT_PATH)/android-qemu2-glue/config/target-$(QEMU2_TARGET) \
    $(ROOT_PATH)/target-$(QEMU2_TARGET_TARGET) \
	$(ROOT_PATH)/tcg \
    $(ROOT_PATH)/tcg/i386 \
    $(QEMU2_GLUE_INCLUDES) \
	$(QEMU2_SDL2_INCLUDES) \

LOCAL_LDFLAGS += $(QEMU2_SYSTEM_LDFLAGS)

LOCAL_LDLIBS += \
    $(ANDROID_EMU_LDLIBS) \
    $(host_common_LDLIBS) \
    $(THRIFT_LDLIBS) \
#    $(QEMU2_SYSTEM_LDLIBS) \
    $(QEMU2_SDL2_LDLIBS) \

#LOCAL_SYMBOL_FILE :=

LOCAL_SRC_FILES := \
    Dump.cpp \
    RemoteRenderer.cpp \
    rpc-thrift/generated/IrrControl.cpp \
    rpc-thrift/generated/main_constants.cpp \
    rpc-thrift/generated/main_types.cpp \
    rpc-thrift/generated/stream_constants.cpp \
    rpc-thrift/generated/StreamControl.cpp \
    rpc-thrift/generated/stream_types.cpp \
    rpc-thrift/overload/TIrrServer.cpp \
    rpc-thrift/overload/TIrrSocket.cpp \
    rpc-thrift/IrrControlHandler.cpp \
    rpc-thrift/IrrRpcMaintainer.cpp \

#LOCAL_INSTALL_DIR := \

IRR_PRIVATE_WHOLE_STATIC_LIBRARIES := $(LOCAL_WHOLE_STATIC_LIBRARIES)
IRR_PRIVATE_STATIC_LIBRARIES := $(LOCAL_STATIC_LIBRARIES)
IRR_PRIVATE_CFLAGS := $(LOCAL_CFLAGS)
IRR_PRIVATE_C_INCLUDES := $(LOCAL_C_INCLUDES)
IRR_PRIVATE_LDFLAGS := $(LOCAL_LDFLAGS)
IRR_PRIVATE_LDLIBS := $(LOCAL_LDLIBS)
IRR_PRIVATE_SYMBOL_FILE := $(LOCAL_SYMBOL_FILE)
IRR_PRIVATE_SRC_FILES := $(LOCAL_SRC_FILES)
IRR_PRIVATE_INSTALL_DIR := $(LOCAL_INSTALL_DIR)

$(call local-link-static-c++lib)
$(call end-emulator-program)
