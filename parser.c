#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "project.h"
static int id_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int id_part(char c) { return isalnum((unsigned char)c) || c == '_'; }
static int is_keyword(const char *w)
{
    static const char *words[] = {"if","else","for","while","switch","case","return","sizeof",
        "do","goto","break","continue","typedef","struct","union","enum","const","volatile",
        "static","extern","register","auto","inline","void","char","short","int","long","float",
        "double","signed","unsigned","bool","true","false","NULL","defined","_Bool",NULL};
    int i; for (i=0; words[i]; ++i) if (!strcmp(w, words[i])) return 1; return 0;
}
static char *mask_source(const char *src, size_t len)
{
    char *out = (char *)malloc(len + 1); size_t i=0; int state=0;
    if (!out) return NULL;
    memcpy(out, src, len); out[len]='\0';
    while (i<len) {
        char c=src[i], next=i+1<len ? src[i+1] : '\0';
        if (!state) {
            if (c=='/' && next=='/') { out[i]=out[i+1]=' '; i+=2; state=1; continue; }
            if (c=='/' && next=='*') { out[i]=out[i+1]=' '; i+=2; state=2; continue; }
            if (c=='"') { out[i++]=' '; state=3; continue; }
            if (c=='\'') { out[i++]=' '; state=4; continue; }
        } else if (state==1) { if (c=='\n') state=0; else out[i]=' '; }
        else if (state==2) {
            if (c=='*' && next=='/') { out[i]=out[i+1]=' '; i+=2; state=0; continue; }
            if (c!='\n') out[i]=' ';
        } else {
            char end = state==3 ? '"' : '\'';
            if (c=='\\' && i+1<len) { if(out[i]!='\n')out[i]=' '; if(out[i+1]!='\n')out[i+1]=' '; i+=2; continue; }
            if (c==end) state=0; 
            if(c!='\n')out[i]=' ';
        }
        ++i;
    }
    return out;
}
static int line_at(const char *s, size_t pos)
{
    int line=1; 
    size_t i;
     for(i=0;i<pos;++i) if(s[i]=='\n') ++line; 
     return line;
}
static void directives(Project *p, int fi, const char *s)
{
    const char *line=s; int line_no=1;
    while (*line) {
        const char *end=strchr(line,'\n'); size_t len=end?(size_t)(end-line):strlen(line);
        const char *original=p->files[fi].source+(line-s);
        if (line[0]=='#') {
            const char *q=line+1; while(*q && isspace((unsigned char)*q)) ++q;
            if (!strncmp(q,"define",6) && isspace((unsigned char)q[6])) {
                char name[SS_MAX_NAME]; size_t n=0; q+=6;
                while(*q && isspace((unsigned char)*q)) ++q;
                while(id_part(*q) && n<sizeof(name)-1) name[n++]=*q++;
                name[n]='\0'; if(n) (void)project_add_symbol(p,name,SYM_MACRO,fi,line_no);
            } else if (!strncmp(q,"include",7)) {
                const char *open=strchr(original,'"'); char close='"'; const char *closep;
                char name[SS_MAX_PATH]; size_t n;
                if(!open) { open=strchr(original,'<'); close='>'; }
                if(open && (closep=strchr(open+1,close))!=NULL) {
                    n=(size_t)(closep-open-1);
                    if(n<sizeof(name)) { memcpy(name,open+1,n); name[n]='\0';
                        (void)project_add_file_edge(p,p->files[fi].path,name,"INCLUDE",line_no); }
                }
            }
        }
        line+=len; if(*line=='\n'){++line;++line_no;} else break;
    }
}
static void find_definitions(Project *p, int fi, const char *s)
{
    size_t i=0,len=strlen(s);
    while(i<len) {
        if(id_start(s[i])) {
            size_t start=i,j,k,n; 
            char name[SS_MAX_NAME]; 
            int depth=0;
            while(i<len && id_part(s[i])){ ++i; }
            n=i-start;
            if(n>=sizeof(name)) 
                continue;
             memcpy(name,s+start,n); 
             name[n]='\0';
            if(is_keyword(name)) continue;
            j=i; while(j<len && isspace((unsigned char)s[j])) ++j;
            if(j>=len || s[j]!='(') continue;
            depth=1;
            k=j+1;
            while(k<len && depth>0){if(s[k]=='(')++depth;
                else if(s[k]==')')--depth;
                ++k;}
            while(k<len && isspace((unsigned char)s[k])) ++k;
            if(depth==0 && k<len && s[k]=='{')
                (void)project_add_symbol(p,name,SYM_FUNCTION,fi,line_at(s,start));
        } else ++i;
    }
}
static void find_calls(Project *p, int fi, const char *s)
{
    size_t i=0,len=strlen(s); 
    int current=-1,brace=0;
    while(i<len) {
        if(id_start(s[i])) {
            size_t start=i,j,k,n; 
            char name[SS_MAX_NAME]; 
            int depth;
            while(i<len && id_part(s[i])) {++i;} 
            n=i-start;
            if(n>=sizeof(name)) continue;
             memcpy(name,s+start,n);
              name[n]='\0';
            j=i; 
            while(j<len && isspace((unsigned char)s[j])) ++j;
            if(j<len && s[j]=='(' && !is_keyword(name)) {
                depth=1;
                k=j+1;
                while(k<len && depth>0){if(s[k]=='(')++depth;else if(s[k]==')')--depth;++k;}
                while(k<len && isspace((unsigned char)s[k])) ++k;
                {
                    int def=project_find_function(p,name,fi);
                    if(def>=0 && depth==0 && k<len && s[k]=='{') current=def;
                    else if(current>=0) (void)project_add_call_edge(p,current,name,fi,line_at(s,start));
                }
            }
        } else {
            if(s[i]=='{') ++brace;
            else if(s[i]=='}') { if(brace>0)--brace; if(brace==0)current=-1; }
            ++i;
        }
    }
}
static void find_simple_variables(Project *p, int fi, const char *s)
{
    const char *line=s; int line_no=1;
    while(*line) {
        const char *end=strchr(line,'\n'); size_t len=end?(size_t)(end-line):strlen(line);
        char buf[2048]; size_t n=len<sizeof(buf)-1?len:sizeof(buf)-1;
        char *q,*semi,*eq,*token,*last=NULL;
        memcpy(buf,line,n);buf[n]='\0';q=buf;while(*q&&isspace((unsigned char)*q))++q;
        if(*q && *q!='#' && strchr(q,';') && !strchr(q,'(') &&
           (!strncmp(q,"int ",4)||!strncmp(q,"char ",5)||!strncmp(q,"float ",6)||
            !strncmp(q,"double ",7)||!strncmp(q,"long ",5)||!strncmp(q,"short ",6)||
            !strncmp(q,"static ",7)||!strncmp(q,"const ",6)||!strncmp(q,"unsigned ",9)||
            !strncmp(q,"volatile ",9))) {
            semi=strchr(q,';');if(semi)*semi='\0';eq=strchr(q,'=');if(eq)*eq='\0';
            token=strtok(q," \t*[],:");
            while(token){if(id_start(token[0])&&!is_keyword(token))last=token;token=strtok(NULL," \t*[],:");}
            if(last)(void)project_add_symbol(p,last,SYM_VARIABLE,fi,line_no);
        }
        line+=len;if(*line=='\n'){++line;++line_no;}else break;
    }
}
void parser_analyze_file(Project *p, int fi)
{
    char *masked=mask_source(p->files[fi].source,p->files[fi].length);
    if(!masked){fprintf(stderr,"Parser allocation failed: %s\n",p->files[fi].path);++p->errors_count;return;}
    directives(p,fi,masked); find_definitions(p,fi,masked); find_calls(p,fi,masked);
    find_simple_variables(p,fi,masked); free(masked);
}
