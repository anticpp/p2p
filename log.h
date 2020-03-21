#ifndef __LOG_H__
#define __LOG_H__
#include <stdio.h>

#define log(fmt, ...) fprintf(stderr, "[%s:%d] "fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define log_trace(fmt, ...) log("[Trace] "fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) log("[Debug] "fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) log("[Info] "fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log("[Error] "fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...) log("[Fatal] "fmt, ##__VA_ARGS__)

#endif
