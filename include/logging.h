#pragma once
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

inline void tlog(const char *fmt, ...) {
  char msg[50];
  time_t now = time(NULL);
  strftime(msg, sizeof msg, "%Y-%m-%d %T", localtime(&now));
  printf("[%s] ", msg);
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}
