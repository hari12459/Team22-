#ifndef SOURCESCOPE_COMMON_H
#define SOURCESCOPE_COMMON_H
#include <stddef.h>
#define SS_MAX_PATH 1024
#define SS_MAX_NAME 128
#define SS_MAX_FILES 2048
#define SS_MAX_SYMBOLS 12000
#define SS_MAX_EDGES 30000
typedef enum { SYM_FUNCTION, SYM_VARIABLE, SYM_MACRO } SymbolType;
typedef struct { char name[SS_MAX_NAME]; SymbolType type; int file_index; int line; } Symbol;
typedef struct { char path[SS_MAX_PATH]; char *source; size_t length; } SourceFile;
typedef struct { char from[SS_MAX_PATH]; char to[SS_MAX_PATH]; char kind[24]; int line; } FileEdge;
typedef struct { int caller_symbol; char callee[SS_MAX_NAME]; int file_index; int line; } CallEdge;
typedef struct Project {
    SourceFile files[SS_MAX_FILES]; int file_count;
    Symbol symbols[SS_MAX_SYMBOLS]; int symbol_count;
    FileEdge file_edges[SS_MAX_EDGES]; int file_edge_count;
    CallEdge call_edges[SS_MAX_EDGES]; int call_edge_count;
    struct BSTNode *symbol_root;
    int errors_count;
} Project;
const char *symbol_type_name(SymbolType type);
#endif
