
#ifndef LOGGER_H
#define LOGGER_H

typedef enum
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

int logger_init(const char *filename);

void logger_log(LogLevel level, const char *format, ...);

void logger_close(void);

#endif
