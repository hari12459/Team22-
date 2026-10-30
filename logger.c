#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "logger.h"

static FILE *log_file = NULL;

static const char *get_level_name(LogLevel level)
{
    switch (level)
    {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
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
    time_t now;
    struct tm *time_info;
    char timestamp[32];
    va_list args;
    va_list copy;

    if (format == NULL)
        return;

    now = time(NULL);
    time_info = localtime(&now);

    if (time_info == NULL)
        return;

    if (strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", time_info) == 0)
        return;

    va_start(args, format);
    va_copy(copy, args);

    if (log_file != NULL)
    {
        fprintf(log_file, "[%s] [%s] ", timestamp, get_level_name(level));
        vfprintf(log_file, format, args);
        fputc('\n', log_file);
        fflush(log_file);
    }

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
