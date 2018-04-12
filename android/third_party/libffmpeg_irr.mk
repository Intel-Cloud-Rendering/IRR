# Build rules for the static ffmpeg prebuilt libraries.
FFMPEG_IRR_TOP_DIR := $(FFMPEG_IRR_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library,\
    emulator-libavcodec-irr,\
    $(FFMPEG_IRR_TOP_DIR)/lib/libavcodec.a)

$(call define-emulator-prebuilt-library,\
    emulator-libavdevice-irr,\
    $(FFMPEG_IRR_TOP_DIR)/lib/libavdevice.a)

$(call define-emulator-prebuilt-library,\
    emulator-libavfilter-irr,\
    $(FFMPEG_IRR_TOP_DIR)/lib/libavfilter.a)

$(call define-emulator-prebuilt-library,\
    emulator-libavformat-irr,\
    $(FFMPEG_IRR_TOP_DIR)/lib/libavformat.a)

$(call define-emulator-prebuilt-library,\
    emulator-libavutil-irr,\
    $(FFMPEG_IRR_TOP_DIR)/lib/libavutil.a)

$(call define-emulator-prebuilt-library,\
    emulator-libswscale-irr,\
    $(FFMPEG_IRR_TOP_DIR)/lib/libswscale.a)

$(call define-emulator-prebuilt-library,\
    emulator-libswresample-irr,\
    $(FFMPEG_IRR_TOP_DIR)/lib/libswresample.a)

FFMPEG_IRR_INCLUDES := $(FFMPEG_IRR_TOP_DIR)/include
FFMPEG_IRR_STATIC_LIBRARIES := \
    emulator-libavdevice-irr \
    emulator-libavformat-irr \
    emulator-libavfilter-irr \
    emulator-libavcodec-irr \
    emulator-libswresample-irr \
    emulator-libswscale-irr \
    emulator-libavutil-irr \

FFMPEG_IRR_LDLIBS := -L$(FFMPEG_IRR_TOP_DIR)/lib -lva -lva-drm -lva-x11
