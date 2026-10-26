#ifndef SOURCESCOPE_ANALYSIS_H
#define SOURCESCOPE_ANALYSIS_H
#include "common.h"
void analysis_print_statistics(const Project *);
void analysis_print_symbols(const Project *);
void analysis_print_dependencies(const Project *);
void analysis_search_symbols(const Project *);
void analysis_impact(const Project *);
void analysis_write_report(const Project *, const char *);
#endif
