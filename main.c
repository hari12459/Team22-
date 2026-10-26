#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "scanner.h"
#include "parser.h"
#include "analysis.h"
#include "project.h"
int main(int argc,char **argv)
{
    Project *p;int i,choice;
    if(argc!=2){printf("Usage: %s <source-directory>\n",argv[0]);return EXIT_FAILURE;}
    p=(Project *)calloc(1,sizeof(*p));if(!p){fputs("Out of memory.\n",stderr);return EXIT_FAILURE;}
    scanner_scan_directory(p,argv[1],0);
    if(!p->file_count){fprintf(stderr,"No .c or .h files found in '%s'.\n",argv[1]);project_cleanup(p);free(p);return EXIT_FAILURE;}
    printf("SourceScope: scanning %d source/header file(s)...\n",p->file_count);
    for(i=0;i<p->file_count;++i){printf("[%d/%d] %s\n",i+1,p->file_count,p->files[i].path);parser_analyze_file(p,i);}
    analysis_print_statistics(p);analysis_print_dependencies(p);
    for(;;){
        puts("\n=== SOURCESCOPE MENU ===\n1. Display all symbols\n2. Search symbol\n3. Analyze change impact\n4. Write report\n5. Exit");
        printf("Choose an option: ");
        if(scanf("%d",&choice)!=1){int ch;while((ch=getchar())!='\n'&&ch!=EOF){}puts("Enter a number from 1 to 5.");continue;}
        switch(choice){
            case 1:analysis_print_symbols(p);break;
            case 2:analysis_search_symbols(p);break;
            case 3:analysis_impact(p);break;
            case 4:analysis_write_report(p,"sourcescope_report.txt");break;
            case 5:project_cleanup(p);free(p);return EXIT_SUCCESS;
            default:puts("Invalid choice.");break;
        }
    }
}
