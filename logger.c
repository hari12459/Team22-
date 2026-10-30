#include <stdio.h>
#include <time.h>
#include "logger.h"

void log_message(LogLevel level, const char *message)
{
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    char timestamp[20];

    if (time_info == NULL) {
        fprintf(stderr, "[UNKNOWN TIME] %s\n", message);
        return;
    }

    strftime(timestamp, sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S", time_info);

    const char *level_text = "INFO";

    if (level == LOG_WARNING)
        level_text = "WARNING";
    else if (level == LOG_ERROR)
        level_text = "ERROR";

    fprintf(stderr, "[%s] [%s] %s\n",
            timestamp, level_text, message);
}
