#ifndef SOURCESCOPE_SCANNER_H
#define SOURCESCOPE_SCANNER_H
#include "common.h"
int scanner_add_file(Project *, const char *);
void scanner_scan_directory(Project *, const char *, int);
#endif
