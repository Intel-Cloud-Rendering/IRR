# Build rules for the static ffmpeg prebuilt libraries.
THRIFT_TOP_DIR := $(THRIFT_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

THRIFT_INCLUDES := $(THRIFT_TOP_DIR)/include
THRIFT_CFLAGS := -I/usr/include

THRIFT_STATIC_LIBRARIES := \

THRIFT_LDLIBS := -L$(THRIFT_TOP_DIR)/lib -lthrift
