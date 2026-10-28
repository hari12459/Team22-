#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
extern "C" {
#include "common.h"
#include "bst.h"
#include "project.h"
#include "scanner.h"
#include "parser.h"
#include "analysis.h"
}

class SourceScopeTest : public ::testing::Test {
protected:
    Project *p;
    void SetUp() override { p = static_cast<Project *>(calloc(1, sizeof(Project))); ASSERT_NE(p, nullptr); }
    void TearDown() override { if (p) { project_cleanup(p); free(p); } }
    void source(const char *s, const char *path="test.c") {
        ASSERT_LT(p->file_count, SS_MAX_FILES);
        int i=p->file_count++;
        snprintf(p->files[i].path, sizeof(p->files[i].path), "%s", path);
        p->files[i].length=strlen(s);
        p->files[i].source=static_cast<char *>(malloc(strlen(s)+1));
        ASSERT_NE(p->files[i].source, nullptr);
        strcpy(p->files[i].source,s);
    }
    std::string tempFile(const char *contents) {
        char path[]="/tmp/sourcescope_test_XXXXXX";
        int fd=mkstemp(path);
        if(fd<0) return "";
        size_t n=strlen(contents);
        bool ok=write(fd,contents,n)==static_cast<ssize_t>(n);
        close(fd);
        if(!ok) { unlink(path); return ""; }
        return path;
    }
};

TEST(BSTTest, EmptyTreeSearch) { EXPECT_EQ(bst_find(nullptr,"x"),-1); }
TEST(BSTTest, FreeEmptyTree) { bst_free(nullptr); SUCCEED(); }
TEST(BSTTest, InsertSingle) { BSTNode *r=bst_insert(nullptr,"main",4); ASSERT_NE(r,nullptr); EXPECT_EQ(bst_find(r,"main"),4); bst_free(r); }
TEST(BSTTest, MissingKey) { BSTNode *r=bst_insert(nullptr,"main",4); ASSERT_NE(r,nullptr); EXPECT_EQ(bst_find(r,"other"),-1); bst_free(r); }
TEST(BSTTest, LeftAndRightChildren) { BSTNode *r=bst_insert(nullptr,"m",1); ASSERT_NE(r,nullptr); bst_insert(r,"a",2); bst_insert(r,"z",3); EXPECT_EQ(bst_find(r,"a"),2); EXPECT_EQ(bst_find(r,"z"),3); bst_free(r); }
TEST(BSTTest, DuplicateKeepsFirstIndex) { BSTNode *r=bst_insert(nullptr,"same",2); ASSERT_NE(r,nullptr); bst_insert(r,"same",8); EXPECT_EQ(bst_find(r,"same"),2); bst_free(r); }
TEST(BSTTest, DeepTree) { BSTNode *r=nullptr; const char *names[]={"d","b","f","a","c","e","g"}; for(int i=0;i<7;i++) r=bst_insert(r,names[i],i); ASSERT_NE(r,nullptr); for(int i=0;i<7;i++) EXPECT_EQ(bst_find(r,names[i]),i); bst_free(r); }

TEST_F(SourceScopeTest, SymbolTypeNames) { EXPECT_STREQ(symbol_type_name(SYM_FUNCTION),"FUNCTION"); EXPECT_STREQ(symbol_type_name(SYM_VARIABLE),"VARIABLE"); EXPECT_STREQ(symbol_type_name(SYM_MACRO),"MACRO"); EXPECT_STREQ(symbol_type_name(static_cast<SymbolType>(99)),"UNKNOWN"); }
TEST_F(SourceScopeTest, AddFunctionSymbol) { EXPECT_EQ(project_add_symbol(p,"main",SYM_FUNCTION,0,1),0); EXPECT_EQ(p->symbol_count,1); EXPECT_STREQ(p->symbols[0].name,"main"); EXPECT_EQ(p->symbols[0].line,1); }
TEST_F(SourceScopeTest, AddVariableAndMacro) { EXPECT_EQ(project_add_symbol(p,"count",SYM_VARIABLE,0,2),0); EXPECT_EQ(project_add_symbol(p,"MAX",SYM_MACRO,0,3),1); EXPECT_EQ(p->symbols[1].type,SYM_MACRO); }
TEST_F(SourceScopeTest, RejectEmptySymbol) { EXPECT_EQ(project_add_symbol(p,"",SYM_FUNCTION,0,1),-1); EXPECT_EQ(p->symbol_count,0); }
TEST_F(SourceScopeTest, RejectNullSymbol) { EXPECT_EQ(project_add_symbol(p,nullptr,SYM_FUNCTION,0,1),-1); }
TEST_F(SourceScopeTest, RejectLongSymbol) { std::string s(SS_MAX_NAME,'x'); EXPECT_EQ(project_add_symbol(p,s.c_str(),SYM_FUNCTION,0,1),-1); }
TEST_F(SourceScopeTest, DuplicateSymbolSameLocation) { EXPECT_EQ(project_add_symbol(p,"run",SYM_FUNCTION,0,2),0); EXPECT_EQ(project_add_symbol(p,"run",SYM_FUNCTION,0,2),0); EXPECT_EQ(p->symbol_count,1); }
TEST_F(SourceScopeTest, SameNameDifferentLocation) { project_add_symbol(p,"run",SYM_FUNCTION,0,2); project_add_symbol(p,"run",SYM_FUNCTION,1,2); project_add_symbol(p,"run",SYM_FUNCTION,0,3); EXPECT_EQ(p->symbol_count,3); }
TEST_F(SourceScopeTest, SameNameDifferentType) { project_add_symbol(p,"item",SYM_FUNCTION,0,1); project_add_symbol(p,"item",SYM_VARIABLE,0,1); EXPECT_EQ(p->symbol_count,2); }
TEST_F(SourceScopeTest, SymbolLimit) { p->symbol_count=SS_MAX_SYMBOLS; EXPECT_EQ(project_add_symbol(p,"extra",SYM_FUNCTION,0,1),-1); p->symbol_count=0; }
TEST_F(SourceScopeTest, FindFunction) { project_add_symbol(p,"work",SYM_FUNCTION,0,1); EXPECT_EQ(project_find_function(p,"work",0),0); EXPECT_EQ(project_find_function(p,"work",1),-1); }
TEST_F(SourceScopeTest, FindFunctionIgnoresVariable) { project_add_symbol(p,"work",SYM_VARIABLE,0,1); EXPECT_EQ(project_find_function(p,"work",0),-1); }
TEST_F(SourceScopeTest, MissingFunction) { EXPECT_EQ(project_find_function(p,"missing",0),-1); }
TEST_F(SourceScopeTest, SymbolIsIndexedInBST) { project_add_symbol(p,"hello",SYM_FUNCTION,0,1); EXPECT_EQ(bst_find(reinterpret_cast<BSTNode *>(p->symbol_root),"hello"),0); }
TEST_F(SourceScopeTest, FileEdge) { EXPECT_EQ(project_add_file_edge(p,"a.c","a.h","INCLUDE",4),1); ASSERT_EQ(p->file_edge_count,1); EXPECT_STREQ(p->file_edges[0].to,"a.h"); EXPECT_EQ(p->file_edges[0].line,4); }
TEST_F(SourceScopeTest, DuplicateFileEdge) { project_add_file_edge(p,"a.c","a.h","INCLUDE",1); project_add_file_edge(p,"a.c","a.h","INCLUDE",5); EXPECT_EQ(p->file_edge_count,1); }
TEST_F(SourceScopeTest, DifferentFileEdges) { project_add_file_edge(p,"a.c","a.h","INCLUDE",1); project_add_file_edge(p,"b.c","a.h","INCLUDE",1); project_add_file_edge(p,"a.c","b.h","INCLUDE",1); EXPECT_EQ(p->file_edge_count,3); }
TEST_F(SourceScopeTest, FileEdgeLimit) { p->file_edge_count=SS_MAX_EDGES; EXPECT_EQ(project_add_file_edge(p,"a","b","INCLUDE",1),0); p->file_edge_count=0; }
TEST_F(SourceScopeTest, CallEdge) { EXPECT_EQ(project_add_call_edge(p,2,"helper",0,8),1); ASSERT_EQ(p->call_edge_count,1); EXPECT_EQ(p->call_edges[0].caller_symbol,2); EXPECT_STREQ(p->call_edges[0].callee,"helper"); }
TEST_F(SourceScopeTest, DuplicateCallEdge) { project_add_call_edge(p,0,"foo",0,3); project_add_call_edge(p,0,"foo",0,3); EXPECT_EQ(p->call_edge_count,1); }
TEST_F(SourceScopeTest, CallsAtDifferentLines) { project_add_call_edge(p,0,"foo",0,3); project_add_call_edge(p,0,"foo",0,4); EXPECT_EQ(p->call_edge_count,2); }
TEST_F(SourceScopeTest, CallEdgeLimit) { p->call_edge_count=SS_MAX_EDGES; EXPECT_EQ(project_add_call_edge(p,0,"foo",0,1),0); p->call_edge_count=0; }
TEST_F(SourceScopeTest, CleanupClearsTree) { project_add_symbol(p,"foo",SYM_FUNCTION,0,1); project_cleanup(p); EXPECT_EQ(p->symbol_root,nullptr); }

TEST_F(SourceScopeTest, ParseFunction) { source("int calculate(void) { return 10; }\n"); parser_analyze_file(p,0); EXPECT_GE(project_find_function(p,"calculate",0),0); }
TEST_F(SourceScopeTest, ParseMultipleFunctions) { source("int add(){return 1;}\nvoid show(){add();}\n"); parser_analyze_file(p,0); EXPECT_GE(project_find_function(p,"add",0),0); EXPECT_GE(project_find_function(p,"show",0),0); }
TEST_F(SourceScopeTest, IgnoreFunctionPrototype) { source("int add(int a, int b);\n"); parser_analyze_file(p,0); EXPECT_EQ(project_find_function(p,"add",0),-1); }
TEST_F(SourceScopeTest, IgnoreCommentedFunction) { source("// int fake(){return 1;}\n/* void hidden() {} */\nint real(){}\n"); parser_analyze_file(p,0); EXPECT_EQ(project_find_function(p,"fake",0),-1); EXPECT_EQ(project_find_function(p,"hidden",0),-1); EXPECT_GE(project_find_function(p,"real",0),0); }
TEST_F(SourceScopeTest, IgnoreFunctionInsideString) { source("char *s=\"int fake(){}\";\nint real(){}\n"); parser_analyze_file(p,0); EXPECT_EQ(project_find_function(p,"fake",0),-1); }
TEST_F(SourceScopeTest, ParseMacro) { source("#define MAX 100\n"); parser_analyze_file(p,0); ASSERT_EQ(p->symbol_count,1); EXPECT_STREQ(p->symbols[0].name,"MAX"); EXPECT_EQ(p->symbols[0].type,SYM_MACRO); }
TEST_F(SourceScopeTest, ParseIncludeQuotes) { source("#include \"math_utils.h\"\n"); parser_analyze_file(p,0); ASSERT_EQ(p->file_edge_count,1); EXPECT_STREQ(p->file_edges[0].to,"math_utils.h"); }
TEST_F(SourceScopeTest, ParseIncludeAngles) { source("#include <stdio.h>\n"); parser_analyze_file(p,0); ASSERT_EQ(p->file_edge_count,1); EXPECT_STREQ(p->file_edges[0].to,"stdio.h"); }
TEST_F(SourceScopeTest, IgnoreCommentedInclude) { source("// #include \"fake.h\"\n"); parser_analyze_file(p,0); EXPECT_EQ(p->file_edge_count,0); }
TEST_F(SourceScopeTest, ParseSimpleVariable) { source("int total = 10;\n"); parser_analyze_file(p,0); ASSERT_EQ(p->symbol_count,1); EXPECT_STREQ(p->symbols[0].name,"total"); EXPECT_EQ(p->symbols[0].type,SYM_VARIABLE); }
TEST_F(SourceScopeTest, ParseMultipleVariableTypes) { source("char letter;\nfloat price;\ndouble value;\nlong count;\n"); parser_analyze_file(p,0); EXPECT_EQ(p->symbol_count,4); }
TEST_F(SourceScopeTest, ParseFunctionCall) { source("void helper(){}\nvoid run(){helper();}\n"); parser_analyze_file(p,0); ASSERT_GE(p->call_edge_count,1); EXPECT_STREQ(p->call_edges[0].callee,"helper"); }
TEST_F(SourceScopeTest, IgnoreCallsInComments) { source("void run(){ // fake();\n return; }\n"); parser_analyze_file(p,0); EXPECT_EQ(p->call_edge_count,0); }
TEST_F(SourceScopeTest, ParseLineNumber) { source("\n\nint work(){}\n"); parser_analyze_file(p,0); int i=project_find_function(p,"work",0); ASSERT_GE(i,0); EXPECT_EQ(p->symbols[i].line,3); }
TEST_F(SourceScopeTest, EmptySource) { source(""); parser_analyze_file(p,0); EXPECT_EQ(p->symbol_count,0); EXPECT_EQ(p->file_edge_count,0); }
TEST_F(SourceScopeTest, ParseTwoFiles) { source("int first(){}\n","first.c"); source("int second(){}\n","second.c"); parser_analyze_file(p,0); parser_analyze_file(p,1); EXPECT_GE(project_find_function(p,"first",0),0); EXPECT_GE(project_find_function(p,"second",1),0); }

TEST_F(SourceScopeTest, ScannerReadsExistingFile) { std::string path=tempFile("int main(){}\n"); ASSERT_FALSE(path.empty()); EXPECT_EQ(scanner_add_file(p,path.c_str()),1); ASSERT_EQ(p->file_count,1); EXPECT_STREQ(p->files[0].source,"int main(){}\n"); EXPECT_EQ(p->files[0].length,13u); unlink(path.c_str()); }
TEST_F(SourceScopeTest, ScannerReadsEmptyFile) { std::string path=tempFile(""); ASSERT_FALSE(path.empty()); EXPECT_EQ(scanner_add_file(p,path.c_str()),1); ASSERT_EQ(p->file_count,1); EXPECT_EQ(p->files[0].length,0u); unlink(path.c_str()); }
TEST_F(SourceScopeTest, ScannerRejectsMissingFile) { EXPECT_EQ(scanner_add_file(p,"/tmp/sourcescope_nonexistent_987654.c"),0); EXPECT_EQ(p->file_count,0); }
TEST_F(SourceScopeTest, ScannerFileLimit) { p->file_count=SS_MAX_FILES; EXPECT_EQ(scanner_add_file(p,"anything.c"),0); p->file_count=0; }
TEST_F(SourceScopeTest, ScannerDirectoryDepthLimit) { scanner_scan_directory(p,"/tmp/nonexistent",33); EXPECT_EQ(p->file_count,0); }
TEST_F(SourceScopeTest, ScannerMissingDirectory) { scanner_scan_directory(p,"/tmp/sourcescope_missing_dir_987654",0); EXPECT_EQ(p->file_count,0); }
TEST_F(SourceScopeTest, ScannerScansSampleProject) { scanner_scan_directory(p,"sample_project",0); EXPECT_GE(p->file_count,3); }

TEST_F(SourceScopeTest, AnalysisStatisticsRuns) { source("int main(){}\n"); parser_analyze_file(p,0); EXPECT_NO_FATAL_FAILURE(analysis_print_statistics(p)); }
TEST_F(SourceScopeTest, AnalysisSymbolsRuns) { project_add_symbol(p,"main",SYM_FUNCTION,0,1); EXPECT_NO_FATAL_FAILURE(analysis_print_symbols(p)); }
TEST_F(SourceScopeTest, AnalysisDependenciesRuns) { project_add_file_edge(p,"a.c","a.h","INCLUDE",1); EXPECT_NO_FATAL_FAILURE(analysis_print_dependencies(p)); }
TEST_F(SourceScopeTest, AnalysisReportCreated) { source("int main(){}\n"); parser_analyze_file(p,0); std::string path=tempFile(""); ASSERT_FALSE(path.empty()); analysis_write_report(p,path.c_str()); FILE *f=fopen(path.c_str(),"rb"); ASSERT_NE(f,nullptr); EXPECT_EQ(fseek(f,0,SEEK_END),0); EXPECT_GT(ftell(f),0); fclose(f); unlink(path.c_str()); }
