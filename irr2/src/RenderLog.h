#ifndef RENDER_LOG
#define RENDER_LOG
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#define LOG_BUF_SZ 2048

#define irr_log_err(fmt, ...) irr::log(__func__, "Error:", fmt, ## __VA_ARGS__)
#define irr_log_warn(fmt, ...) irr::log(__func__, "Warning:", fmt, ## __VA_ARGS__)
#define irr_log_info(fmt, ...) irr::log(__func__, "Info:", fmt, ## __VA_ARGS__)
#define irr_assert assert
#define irr_log_plain printf

namespace irr {
  void log(const char *func, const char *msg, const char *fmt, ...);
}

#endif
