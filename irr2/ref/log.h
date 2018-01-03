#ifndef _GF_LOG_H
#define _GF_LOG_H
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#define LOG_BUF_SZ 2048

#define gf_log_error(fmt, ...) gf::log(__func__, "Error:", fmt, ## __VA_ARGS__)
#define gf_log_warning(fmt, ...) gf::log(__func__, "Warning:", fmt, ## __VA_ARGS__)
#define gf_log_info(fmt, ...) gf::log(__func__, "Info:", fmt, ## __VA_ARGS__)
#define gf_assert assert

namespace gf {
  void log(const char *func, const char *msg, const char *fmt, ...);
  void set_start_time_ms();
  void get_elapsed_time_ms(const char *msg);
  class PerfMonitor
  {
 public:
    PerfMonitor(bool stat);
    ~PerfMonitor();
    void start_timestamp_ms();
    double stop_timestamp_ms();
    void stop_timestamp_ms_and_log(const char *msg);
    void log_statistics();
 private:
    struct timespec start;
    struct timespec stop;
    double total;
    double min;
    double max;
    long counter;
    bool stat;
  };
}

#endif
