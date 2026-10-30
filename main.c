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

    if (argc != 2)
    {
        printf("Usage: %s <source-directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!logger_init("sourcescope.log"))
    {
        fprintf(stderr, "Failed to initialize sourcescope.log in the current working directory\n");
        return EXIT_FAILURE;
    }

    logger_log(LOG_INFO, "SourceScope application started");

    p = (Project *)calloc(1, sizeof(*p));
    if (p == NULL)
    {
        logger_log(LOG_ERROR, "Memory allocation failed");
        logger_close();
        fputs("Out of memory.\n", stderr);
        return EXIT_FAILURE;
    }

    logger_log(LOG_INFO, "Scanning source directory: %s", argv[1]);
    scanner_scan_directory(p, argv[1], 0);

    if (!p->file_count)
    {
        logger_log(LOG_ERROR, "No .c or .h files found in '%s'", argv[1]);
        fprintf(stderr, "No .c or .h files found in '%s'.\n", argv[1]);
        project_cleanup(p);
        free(p);
        logger_close();
        return EXIT_FAILURE;
    }

    printf("SourceScope: scanning %d source/header file(s)...\n", p->file_count);

    for (i = 0; i < p->file_count; ++i)
    {
        printf("[%d/%d] %s\n", i + 1, p->file_count, p->files[i].path);
        logger_log(LOG_INFO, "Analyzing file: %s", p->files[i].path);
        parser_analyze_file(p, i);
    }

    logger_log(LOG_INFO, "Analysis completed. Files: %d, symbols: %d, include edges: %d, call edges: %d",
               p->file_count, p->symbol_count, p->file_edge_count, p->call_edge_count);

    analysis_print_statistics(p);
    analysis_print_dependencies(p);

    for (;;)
    {
        puts("\n=== SOURCESCOPE MENU ===\n1. Display all symbols\n2. Search symbol\n3. Analyze change impact\n4. Write report\n5. Exit");
        printf("Choose an option: ");

        if (scanf("%d", &choice) != 1)
        {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            puts("Enter a number from 1 to 5.");
            continue;
        }

        switch (choice)
        {
            case 1:
                analysis_print_symbols(p);
                break;
            case 2:
                analysis_search_symbols(p);
                break;
            case 3:
                analysis_impact(p);
                break;
            case 4:
                analysis_write_report(p, "sourcescope_report.txt");
                logger_log(LOG_INFO, "Analysis report generated");
                break;
            case 5:
                logger_log(LOG_INFO, "SourceScope application stopped");
                project_cleanup(p);
                free(p);
                logger_close();
                return EXIT_SUCCESS;
            default:
                puts("Invalid choice.");
                break;
        }
    }
}
