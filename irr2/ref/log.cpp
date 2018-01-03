#include "gf_log.h"
#include <limits>
#define THOUSAND 1000.0;
#define MILLION 1000000.0;
#define BILLION 1000000000.0;

void gf::log(const char *func, const char *msg, const char *fmt, ...) {
  char buf[LOG_BUF_SZ];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  printf("[%s] %s %s\n", func, msg, buf);
}

gf::PerfMonitor::PerfMonitor(bool stat) :
    stat(stat) {
  start.tv_sec = 0;
  start.tv_nsec = 0;
  stop.tv_sec = 0;
  stop.tv_nsec = 0;
  total = 0;
  counter = 0;
  max = 0;
  min = std::numeric_limits<double>::max();
}

gf::PerfMonitor::~PerfMonitor() {
}

void gf::PerfMonitor::start_timestamp_ms() {
  if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
    gf_log_error("clock gettime\n");
  }
}

double gf::PerfMonitor::stop_timestamp_ms() {
  double elapsed, diff_sec, diff_nsec;
  if (start.tv_sec == 0 || start.tv_nsec == 0) {
    gf_log_error("start time not set\n");
    return 0;
  }
  if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
    gf_log_error("clock gettime\n");
    return 0;
  }
  diff_sec = ((double)stop.tv_sec - (double)start.tv_sec) * THOUSAND;
  diff_nsec = ((double)stop.tv_nsec - (double)start.tv_nsec) / MILLION;
  elapsed = diff_sec + diff_nsec;
  if (stat) {
    total += elapsed;
    counter++;
    if (elapsed < min) {
      min = elapsed;
    }
    if (elapsed > max) {
      max = elapsed;
    }
  }
  start.tv_sec = stop.tv_sec;
  start.tv_nsec = stop.tv_nsec;
  return (diff_sec + diff_nsec);
}

void gf::PerfMonitor::stop_timestamp_ms_and_log(const char *msg) {
  gf_log_info("%s %.2f\n", msg, stop_timestamp_ms());
}

void gf::PerfMonitor::log_statistics() {
  if (stat) {
    gf_log_info("stat: min = %.2f, max = %.2f, avg = %.2f\n", min, max, total/counter);
  }
}
