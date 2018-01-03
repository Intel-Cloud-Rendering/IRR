#include "RenderDispatch.h"
#include "emugl/common/shared_library.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "RenderLog.h"

/* global dispatch functions */
EGLDispatch s_egl;
GLESv2Dispatch s_gles2;
GLESv1Dispatch s_gles1;

#define DEFAULT_EGL_LIB "lib64EGL_translator"

#define RENDER_EGL_LOAD_FIELD(return_type, function_name, signature) \
  s_egl. function_name = (function_name ## _t) lib->findSymbol(#function_name);

#define RENDER_EGL_LOAD_OPTIONAL_FIELD(return_type, function_name, signature) \
  if (s_egl.eglGetProcAddress) \
    s_egl. function_name = (function_name ## _t) s_egl.eglGetProcAddress(#function_name); \
  if (!s_egl.function_name || !s_egl.eglGetProcAddress) \
    RENDER_EGL_LOAD_FIELD(return_type, function_name, signature)

bool init_egl_dispatch() {
  const char *libName = DEFAULT_EGL_LIB;
  char error[256];
  emugl::SharedLibrary *lib = emugl::SharedLibrary::open(libName, error, sizeof(error));
  if (!lib) {
    irr_log_err("Failed to open %s: [%s]\n", libName, error);
    return false;
  }
  LIST_RENDER_EGL_FUNCTIONS(RENDER_EGL_LOAD_FIELD)
  LIST_RENDER_EGL_EXTENSIONS_FUNCTIONS(RENDER_EGL_LOAD_OPTIONAL_FIELD)

  return true;
}

bool init_gles1_dispatch() {
  if (!gles1_dispatch_init(&s_gles1))
    return false;
  return true;
}

bool init_gles2_dispatch() {
  if (!gles2_dispatch_init(&s_gles2))
    return false;
  return true;
}
