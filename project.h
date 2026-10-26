#ifndef SOURCESCOPE_PROJECT_H
#define SOURCESCOPE_PROJECT_H
#include "common.h"
int project_add_symbol(Project *, const char *, SymbolType, int, int);
int project_add_file_edge(Project *, const char *, const char *, const char *, int);
int project_add_call_edge(Project *, int, const char *, int, int);
int project_find_function(const Project *, const char *, int);
void project_cleanup(Project *);
#endif
