// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/OpenGLESHostServer.h"

#include "android/base/files/StdioStream.h"
#include "android/base/files/Stream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"


#include "android/opengl/OpenGLESHostListener.h"
#include "android/utils/debug.h"

#include <memory>
#include <string>

#include <stdio.h>
#include <stdlib.h>

namespace {

using android::opengl::OpenGLESHostListener;

// Global variables used here.
struct Globals {
    Globals() : hostListener() {}

    void registerServices() {

    }

    OpenGLESHostListener hostListener;
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

}  // namespace

int android_opengles_server_init(int port) {
    auto globals = sGlobals.ptr();
    if (!globals->hostListener.reset(port)) {
        return -1;
    }

    globals->hostListener.startListening();
    return 0;
}

void android_opengles_server_undo_init(void) {
    sGlobals->hostListener.reset(-1);
}

void android_opengles_service_init(void) {
    // Register adb pipe service.
    //sGlobals->registerServices();
}

