#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>   /* for gettimeofday – milliseconds */
#include "logger.h"

static FILE *log_file = NULL;

static const char *get_level_name(LogLevel level)
{
    switch (level)
    {
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR:   return "ERROR";
        default:          return "UNKNOWN";
    }
}

int logger_init(const char *filename)
{
    if (filename == NULL)
        return 0;

    if (log_file != NULL)
        logger_close();

    log_file = fopen(filename, "a");
    if (log_file == NULL)
        return 0;

    return 1;
}

void logger_log(LogLevel level, const char *format, ...)
{
    struct timeval tv;
    struct tm *time_info;
    char timestamp[40];          /* a bit larger for .mmm */
    va_list args;
    va_list copy;

    if (format == NULL)
        return;

    /* Get time with milliseconds */
    if (gettimeofday(&tv, NULL) != 0) {
        /* fallback if gettimeofday fails */
        time_t now = time(NULL);
        time_info = localtime(&now);
        if (time_info)
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S.000", time_info);
        else
            snprintf(timestamp, sizeof(timestamp), "????-??-?? ??:??:??.???");
    } else {
        time_info = localtime(&tv.tv_sec);
        if (time_info == NULL) {
            snprintf(timestamp, sizeof(timestamp), "????-??-?? ??:??:??.???");
        } else {
            /* Format: 2026-10-30 13:28:45.123 */
            size_t n = strftime(timestamp, sizeof(timestamp),
                                "%Y-%m-%d %H:%M:%S", time_info);
            if (n > 0 && n < sizeof(timestamp) - 5) {
                snprintf(timestamp + n, sizeof(timestamp) - n,
                         ".%03ld", (long)(tv.tv_usec / 1000));
            }
        }
    }

    va_start(args, format);
    va_copy(copy, args);

    /* Write to log file */
    if (log_file != NULL)
    {
        fprintf(log_file, "[%s] [%s] ", timestamp, get_level_name(level));
        vfprintf(log_file, format, args);
        fputc('\n', log_file);
        fflush(log_file);
    }

    /* Write to console */
    printf("[%s] [%s] ", timestamp, get_level_name(level));
    vprintf(format, copy);
    putchar('\n');
    fflush(stdout);

    va_end(copy);
    va_end(args);
}

void logger_close(void)
{
    if (log_file != NULL)
    {
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
}
