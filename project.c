#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "bst.h"
const char *symbol_type_name(SymbolType type)
{
    switch (type) {
        case SYM_FUNCTION: return "FUNCTION";
        case SYM_VARIABLE: return "VARIABLE";
        case SYM_MACRO: return "MACRO";
        default: return "UNKNOWN";
    }
}
int project_add_symbol(Project *p, const char *name, SymbolType type, int fi, int line)
{
    int i;
    if (!name || !*name || strlen(name) >= SS_MAX_NAME || p->symbol_count >= SS_MAX_SYMBOLS) return -1;
    for (i = 0; i < p->symbol_count; ++i)
        if (p->symbols[i].type == type && p->symbols[i].file_index == fi &&
            p->symbols[i].line == line && !strcmp(p->symbols[i].name, name)) return i;
    i = p->symbol_count;
    snprintf(p->symbols[i].name, sizeof(p->symbols[i].name), "%s", name);
    p->symbols[i].type = type; p->symbols[i].file_index = fi; p->symbols[i].line = line;
    {
        BSTNode *new_root = bst_insert((BSTNode *)p->symbol_root, name, i);
        if (!new_root) return -1;
        p->symbol_root = (struct BSTNode *)new_root;
    }
    ++p->symbol_count;
    return i;
}
int project_add_file_edge(Project *p, const char *from, const char *to, const char *kind, int line)
{
    int i;
    for (i = 0; i < p->file_edge_count; ++i)
        if (!strcmp(p->file_edges[i].from, from) && !strcmp(p->file_edges[i].to, to) &&
            !strcmp(p->file_edges[i].kind, kind)) return 1;
    if (p->file_edge_count >= SS_MAX_EDGES) return 0;
    snprintf(p->file_edges[i].from, sizeof(p->file_edges[i].from), "%s", from);
    snprintf(p->file_edges[i].to, sizeof(p->file_edges[i].to), "%s", to);
    snprintf(p->file_edges[i].kind, sizeof(p->file_edges[i].kind), "%s", kind);
    p->file_edges[i].line = line; ++p->file_edge_count; return 1;
}
int project_add_call_edge(Project *p, int caller, const char *callee, int fi, int line)
{
    int i;
    for (i = 0; i < p->call_edge_count; ++i)
        if (p->call_edges[i].caller_symbol == caller && p->call_edges[i].file_index == fi &&
            p->call_edges[i].line == line && !strcmp(p->call_edges[i].callee, callee)) return 1;
    if (p->call_edge_count >= SS_MAX_EDGES) return 0;
    p->call_edges[i].caller_symbol = caller;
    snprintf(p->call_edges[i].callee, sizeof(p->call_edges[i].callee), "%s", callee);
    p->call_edges[i].file_index = fi; p->call_edges[i].line = line;
    ++p->call_edge_count; return 1;
}
int project_find_function(const Project *p, const char *name, int fi)
{
    int i;
    for (i = 0; i < p->symbol_count; ++i)
        if (p->symbols[i].type == SYM_FUNCTION && p->symbols[i].file_index == fi &&
            !strcmp(p->symbols[i].name, name)) return i;
    return -1;
}
void project_cleanup(Project *p)
{
    int i;
    for (i = 0; i < p->file_count; ++i) free(p->files[i].source);
    bst_free((BSTNode *)p->symbol_root); p->symbol_root = NULL;
}
