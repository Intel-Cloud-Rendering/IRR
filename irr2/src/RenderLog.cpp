#include "RenderLog.h"

void irr::log(const char *func, const char *msg, const char *fmt, ...) {
  char buf[LOG_BUF_SZ];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  printf("[%s] %s %s\n", func, msg, buf);
}
