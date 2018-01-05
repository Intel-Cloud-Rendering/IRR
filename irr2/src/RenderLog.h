#ifndef RENDER_LOG
#define RENDER_LOG
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#define LOG_BUF_SZ 2048

//#define DEBUG_LOG

#ifdef DEBUG_LOG
#define irr_log_err(fmt, ...) irr::log(__func__, "Error:", fmt, ## __VA_ARGS__)
#define irr_log_warn(fmt, ...) irr::log(__func__, "Warning:", fmt, ## __VA_ARGS__)
#define irr_log_info(fmt, ...) irr::log(__func__, "Info:", fmt, ## __VA_ARGS__)
#define irr_log_stat irr_log_info
#define irr_assert assert
#define irr_log_plain printf
#else
#define irr_log_err
#define irr_log_warn
#define irr_log_info
#define irr_log_stat(fmt, ...) irr::log(__func__, "Stat:", fmt, ## __VA_ARGS__)
#define irr_assert
#define irr_log_plain
#endif

namespace irr {
  void log(const char *func, const char *msg, const char *fmt, ...);
}

#endif
