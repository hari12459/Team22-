#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "scanner.h"
#include "parser.h"
#include "analysis.h"
#include "project.h"
#include "logger.h"

int main(int argc, char **argv)
{
    Project *p;
    int i, choice;

    log_message(LOG_INFO, "SourceScope started");

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source-directory>\n", argv[0]);
        log_message(LOG_ERROR, "Invalid arguments provided");
        return EXIT_FAILURE;
    }

    p = (Project *)calloc(1, sizeof(*p));

    if (!p) {
        fputs("Out of memory.\n", stderr);
        log_message(LOG_ERROR, "Failed to allocate project memory");
        return EXIT_FAILURE;
    }

    log_message(LOG_INFO, "Scanning source directory");
    scanner_scan_directory(p, argv[1], 0);

    if (!p->file_count) {
        fprintf(stderr, "No .c or .h files found in '%s'.\n", argv[1]);
        log_message(LOG_WARNING, "No source or header files found");
        project_cleanup(p);
        free(p);
        log_message(LOG_INFO, "SourceScope stopped");
        return EXIT_FAILURE;
    }

    printf("SourceScope: scanning %d source/header file(s)...\n",
           p->file_count);

    for (i = 0; i < p->file_count; ++i) {
        printf("[%d/%d] %s\n",
               i + 1, p->file_count, p->files[i].path);

        log_message(LOG_INFO, "Analyzing source file");
        parser_analyze_file(p, i);
    }

    log_message(LOG_INFO, "Source file analysis completed");

    analysis_print_statistics(p);
    analysis_print_dependencies(p);

    for (;;) {
        puts("\n=== SOURCESCOPE MENU ===\n"
             "1. Display all symbols\n"
             "2. Search symbol\n"
             "3. Analyze change impact\n"
             "4. Write report\n"
             "5. Exit");

        printf("Choose an option: ");

        if (scanf("%d", &choice) != 1) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            puts("Enter a number from 1 to 5.");
            log_message(LOG_WARNING, "Invalid menu input");
            continue;
        }

        switch (choice) {
            case 1:
                log_message(LOG_INFO, "Displaying symbols");
                analysis_print_symbols(p);
                break;

            case 2:
                log_message(LOG_INFO, "Searching symbols");
                analysis_search_symbols(p);
                break;

            case 3:
                log_message(LOG_INFO, "Analyzing change impact");
                analysis_impact(p);
                break;

            case 4:
                log_message(LOG_INFO, "Writing SourceScope report");
                analysis_write_report(p, "sourcescope_report.txt");
                log_message(LOG_INFO, "Report generation requested");
                break;

            case 5:
                log_message(LOG_INFO, "Cleaning up project resources");
                project_cleanup(p);
                free(p);
                log_message(LOG_INFO, "SourceScope stopped");
                return EXIT_SUCCESS;

            default:
                puts("Invalid choice.");
                log_message(LOG_WARNING, "Invalid menu choice");
                break;
        }
    }
}
