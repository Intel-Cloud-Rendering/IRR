$(call define-emulator-prebuilt-library,\
    emulator-libtrans,\
    $(LIBTRANS_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/libtrans.a)

LIBTRANS_INCLUDES := $(LIBTRANS_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/include
LIBTRANS_LDLIBS := -ldrm -lva -lva-drm -lva-x11 -lmfx -L$(LIBTRANS_PREBUILTS_DIR)/../mfx/$(BUILD_TARGET_TAG)/lib/
