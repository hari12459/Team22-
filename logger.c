
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "logger.h"

static FILE *log_file = NULL;

static const char *get_level_name(LogLevel level)
{
    switch (level)
    {
        case LOG_DEBUG:
            return "DEBUG";

        case LOG_INFO:
            return "INFO";

        case LOG_WARNING:
            return "WARNING";

        case LOG_ERROR:
            return "ERROR";

        default:
            return "UNKNOWN";
    }
}

int logger_init(const char *filename)
{
    if (filename == NULL)
        return 0;

    log_file = fopen(filename, "a");

    if (log_file == NULL)
        return 0;

    return 1;
}

void logger_log(LogLevel level, const char *format, ...)
{
    if (log_file == NULL || format == NULL)
        return;

    struct timespec ts;
    struct tm time_info;

    timespec_get(&ts, TIME_UTC);

    localtime_r(&ts.tv_sec, &time_info);

    fprintf(log_file,
            "[%04d-%02d-%02d %02d:%02d:%02d.%03ld] [%s] ",
            time_info.tm_year + 1900,
            time_info.tm_mon + 1,
            time_info.tm_mday,
            time_info.tm_hour,
            time_info.tm_min,
            time_info.tm_sec,
            ts.tv_nsec / 1000000,
            get_level_name(level));

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");

    fflush(log_file);
}

void logger_close(void)
{
    if (log_file != NULL)
    {
        fclose(log_file);
        log_file = NULL;
    }
}
