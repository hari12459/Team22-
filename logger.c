#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>   /* gettimeofday for milliseconds */
#include "logger.h"

static FILE *log_file = NULL;

static const char *get_level_name(LogLevel level)
{
    switch (level) {
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
    return log_file != NULL;
}

void logger_log(LogLevel level, const char *format, ...)
{
    struct timeval tv;
    struct tm tm_buf;
    char timestamp[40];
    va_list args, copy;

    if (format == NULL)
        return;

    /* Always produce a timestamp – never silently drop the message */
    if (gettimeofday(&tv, NULL) == 0 && localtime_r(&tv.tv_sec, &tm_buf) != NULL) {
        size_t n = strftime(timestamp, sizeof(timestamp),
                            "%Y-%m-%d %H:%M:%S", &tm_buf);
        if (n > 0 && n < sizeof(timestamp) - 5)
            snprintf(timestamp + n, sizeof(timestamp) - n,
                     ".%03ld", (long)(tv.tv_usec / 1000));
        else
            snprintf(timestamp, sizeof(timestamp), "%ld.%03ld",
                     (long)tv.tv_sec, (long)(tv.tv_usec / 1000));
    } else {
        /* fallback */
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        if (t)
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S.000", t);
        else
            snprintf(timestamp, sizeof(timestamp), "0000-00-00 00:00:00.000");
    }

    va_start(args, format);
    va_copy(copy, args);

    /* Log file */
    if (log_file != NULL) {
        fprintf(log_file, "[%s] [%s] ", timestamp, get_level_name(level));
        vfprintf(log_file, format, args);
        fputc('\n', log_file);
        fflush(log_file);
    }

    /* Console */
    printf("[%s] [%s] ", timestamp, get_level_name(level));
    vprintf(format, copy);
    putchar('\n');
    fflush(stdout);

    va_end(copy);
    va_end(args);
}

void logger_close(void)
{
    if (log_file != NULL) {
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
}
