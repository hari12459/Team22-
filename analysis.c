#include <stdio.h>
#include <string.h>
#include "analysis.h"
#include "bst.h"
void analysis_print_statistics(const Project *p)
{
    int i,fn=0,var=0,mac=0;
    for(i=0;i<p->symbol_count;++i){if(p->symbols[i].type==SYM_FUNCTION)++fn;
        else if(p->symbols[i].type==SYM_VARIABLE)++var;else if(p->symbols[i].type==SYM_MACRO)++mac;}
    puts("\n=== PROJECT STATISTICS ===");
    printf("Files: %d\nFunctions: %d\nProbable variables: %d\nMacros: %d\nInclude edges: %d\nCall edges: %d\nWarnings: %d\n",
        p->file_count,fn,var,mac,p->file_edge_count,p->call_edge_count,p->errors_count);
}
void analysis_print_symbols(const Project *p)
{
    int i; puts("\n=== SYMBOL INDEX ===");
    for(i=0;i<p->symbol_count;++i){const Symbol *s=&p->symbols[i];
        printf("%-8s %-28s %s:%d\n",symbol_type_name(s->type),s->name,p->files[s->file_index].path,s->line);}
}
void analysis_print_dependencies(const Project *p)
{
    int i; puts("\n=== FILE INCLUDE DEPENDENCIES ===");
    for(i=0;i<p->file_edge_count;++i){const FileEdge *e=&p->file_edges[i];
        printf("%s --[%s]--> %s (line %d)\n",e->from,e->kind,e->to,e->line);}
    puts("\n=== FUNCTION CALL RELATIONSHIPS ===");
    for(i=0;i<p->call_edge_count;++i){const CallEdge *e=&p->call_edges[i];
        const char *caller=e->caller_symbol>=0?p->symbols[e->caller_symbol].name:"<unknown>";
        printf("%s -> %s (%s:%d)\n",caller,e->callee,p->files[e->file_index].path,e->line);}
}
void analysis_search_symbols(const Project *p)
{
    char query[SS_MAX_NAME]; int i,found=0,index;
    printf("Enter symbol name: ");if(scanf("%127s",query)!=1)return;
    index=bst_find((const BSTNode *)p->symbol_root,query);
    if(index>=0){const Symbol *s=&p->symbols[index];
        printf("BST match: %s (%s), %s:%d\n",s->name,symbol_type_name(s->type),p->files[s->file_index].path,s->line);}
    else puts("No exact BST match.");
    puts("Matching entries:");
    for(i=0;i<p->symbol_count;++i)if(strstr(p->symbols[i].name,query)){
        printf("  %-8s %-24s %s:%d\n",symbol_type_name(p->symbols[i].type),p->symbols[i].name,
            p->files[p->symbols[i].file_index].path,p->symbols[i].line);++found;}
    if(!found)puts("  No symbols found.");
}
static const char *base_name(const char *path)
{
    const char *a=strrchr(path,'/'),*b=strrchr(path,'\\');if(!a||(b&&b>a))a=b;return a?a+1:path;
}
static int path_matches(const char *path,const char *query)
{
    return !strcmp(path,query)||!strcmp(base_name(path),query)||strstr(path,query)!=NULL;
}
void analysis_impact(const Project *p)
{
    char query[SS_MAX_PATH];int visited[SS_MAX_FILES]={0},queue[SS_MAX_FILES];
    int head=0,tail=0,i,found=0;
    printf("Enter changed file name/path: ");if(scanf("%1023s",query)!=1)return;
    for(i=0;i<p->file_count;++i)if(path_matches(p->files[i].path,query)){
        visited[i]=1;queue[tail++]=i;found=1;}
    if(!found){printf("No scanned file matches '%s'.\n",query);return;}
    while(head<tail){
        int changed=queue[head++];const char *changed_path=p->files[changed].path;
        const char *changed_base=base_name(changed_path);
        for(i=0;i<p->file_edge_count;++i){
            const FileEdge *e=&p->file_edges[i];int f;
            if(strcmp(e->to,changed_path)&&strcmp(e->to,changed_base))continue;
            for(f=0;f<p->file_count;++f)if(!visited[f]&&!strcmp(p->files[f].path,e->from)&&tail<SS_MAX_FILES){
                visited[f]=1;queue[tail++]=f;}
        }
    }
    puts("Potentially affected files (includes changed file):");
    for(i=0;i<p->file_count;++i)if(visited[i])printf("  %s\n",p->files[i].path);
    puts("Note: approximate results; include resolution uses file names.");
}
void analysis_write_report(const Project *p,const char *path)
{
    FILE *fp=fopen(path,"w");int i;if(!fp){perror("Could not write report");return;}
    fprintf(fp,"SourceScope Analysis Report\n===========================\nFiles: %d\nSymbols: %d\nInclude edges: %d\nCall edges: %d\n",
        p->file_count,p->symbol_count,p->file_edge_count,p->call_edge_count);
    fprintf(fp,"\nSYMBOLS\n-------\n");
    for(i=0;i<p->symbol_count;++i){const Symbol *s=&p->symbols[i];
        fprintf(fp,"%s,%s,%s,%d\n",symbol_type_name(s->type),s->name,p->files[s->file_index].path,s->line);}
    fprintf(fp,"\nFILE DEPENDENCIES\n-----------------\n");
    for(i=0;i<p->file_edge_count;++i)fprintf(fp,"%s -> %s (line %d)\n",p->file_edges[i].from,p->file_edges[i].to,p->file_edges[i].line);
    fprintf(fp,"\nFUNCTION CALLS\n--------------\n");
    for(i=0;i<p->call_edge_count;++i){const CallEdge *e=&p->call_edges[i];
        const char *caller=e->caller_symbol>=0?p->symbols[e->caller_symbol].name:"<unknown>";
        fprintf(fp,"%s -> %s (%s:%d)\n",caller,e->callee,p->files[e->file_index].path,e->line);}
    fclose(fp);printf("Report written to %s\n",path);
}
