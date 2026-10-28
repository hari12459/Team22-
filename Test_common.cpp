
#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

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
    Project *p = nullptr;

    void SetUp() override {
        p = (Project*)calloc(1, sizeof(Project));
        ASSERT_NE(p, nullptr);
    }

    void TearDown() override {
        if (p) {
            project_cleanup(p);
            free(p);
        }
    }

    void source(const char *text, const char *path = "test.c") {
        ASSERT_LT(p->file_count, SS_MAX_FILES);
        SourceFile *f = &p->files[p->file_count++];
        snprintf(f->path, sizeof(f->path), "%s", path);
        f->length = strlen(text);
        f->source = (char*)malloc(f->length + 1);
        ASSERT_NE(f->source, nullptr);
        memcpy(f->source, text, f->length + 1);
    }

    static void writeFile(const std::string &name, const std::string &data) {
        std::ofstream out(name, std::ios::binary);
        out << data;
    }

    static std::string readFile(const std::string &name) {
        std::ifstream in(name);
        return std::string(
            (std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>()
        );
    }

    static std::string withInput(const std::string &input,
                                 void (*fn)(const Project*),
                                 const Project *p) {
        FILE *tmp = tmpfile();
        if (!tmp) return "";

        fwrite(input.data(), 1, input.size(), tmp);
        rewind(tmp);

        int saved = dup(STDIN_FILENO);
        if (saved < 0) {
            fclose(tmp);
            return "";
        }

        fflush(stdout);
        dup2(fileno(tmp), STDIN_FILENO);
        clearerr(stdin);

        testing::internal::CaptureStdout();
        fn(p);
        fflush(stdout);

        std::string result =
            testing::internal::GetCapturedStdout();

        dup2(saved, STDIN_FILENO);
        close(saved);
        clearerr(stdin);
        fclose(tmp);

        return result;
    }
};

// BST TEST CASES

TEST_F(SourceScopeTest, BSTInsertAndFind) {
    BSTNode *r = bst_insert(nullptr, "main", 0);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(bst_find(r, "main"), 0);
    bst_free(r);
}

TEST_F(SourceScopeTest, BSTMissingKey) {
    BSTNode *r = bst_insert(nullptr, "main", 0);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(bst_find(r, "missing"), -1);
    bst_free(r);
}

TEST_F(SourceScopeTest, BSTLeftAndRightChildren) {
    BSTNode *r = bst_insert(nullptr, "middle", 1);
    ASSERT_NE(r, nullptr);
    r = bst_insert(r, "alpha", 2);
    r = bst_insert(r, "zeta", 3);

    EXPECT_EQ(bst_find(r, "alpha"), 2);
    EXPECT_EQ(bst_find(r, "zeta"), 3);
    bst_free(r);
}

TEST_F(SourceScopeTest, BSTDuplicateKeepsOriginalIndex) {
    BSTNode *r = bst_insert(nullptr, "same", 4);
    ASSERT_NE(r, nullptr);
    r = bst_insert(r, "same", 9);
    EXPECT_EQ(bst_find(r, "same"), 4);
    bst_free(r);
}

TEST_F(SourceScopeTest, BSTNullRootIsSafe) {
    EXPECT_EQ(bst_find(nullptr, "anything"), -1);
    bst_free(nullptr);
}

// PROJECT MODULE TEST CASES

TEST_F(SourceScopeTest, AddFunctionSymbol) {
    EXPECT_EQ(project_add_symbol(p, "main", SYM_FUNCTION, 0, 1), 0);
    ASSERT_EQ(p->symbol_count, 1);
    EXPECT_EQ(p->symbols[0].type, SYM_FUNCTION);
}

TEST_F(SourceScopeTest, RejectEmptySymbol) {
    EXPECT_EQ(project_add_symbol(p, "", SYM_FUNCTION, 0, 1), -1);
    EXPECT_EQ(p->symbol_count, 0);
}

TEST_F(SourceScopeTest, RejectLongSymbol) {
    std::string name(SS_MAX_NAME, 'x');
    EXPECT_EQ(project_add_symbol(p, name.c_str(), SYM_FUNCTION, 0, 1), -1);
}

TEST_F(SourceScopeTest, DuplicateSymbolSameLocation) {
    EXPECT_EQ(project_add_symbol(p, "x", SYM_VARIABLE, 0, 3), 0);
    EXPECT_EQ(project_add_symbol(p, "x", SYM_VARIABLE, 0, 3), 0);
    EXPECT_EQ(p->symbol_count, 1);
}

TEST_F(SourceScopeTest, SameSymbolDifferentLines) {
    EXPECT_EQ(project_add_symbol(p, "x", SYM_VARIABLE, 0, 3), 0);
    EXPECT_EQ(project_add_symbol(p, "x", SYM_VARIABLE, 0, 4), 1);
}

TEST_F(SourceScopeTest, AddFileDependency) {
    EXPECT_EQ(project_add_file_edge(
        p, "main.c", "util.h", "INCLUDE", 1), 1);

    ASSERT_EQ(p->file_edge_count, 1);
    EXPECT_STREQ(p->file_edges[0].to, "util.h");
}

TEST_F(SourceScopeTest, DuplicateFileDependency) {
    project_add_file_edge(p, "main.c", "util.h", "INCLUDE", 1);

    EXPECT_EQ(project_add_file_edge(
        p, "main.c", "util.h", "INCLUDE", 9), 1);

    EXPECT_EQ(p->file_edge_count, 1);
}

TEST_F(SourceScopeTest, AddFunctionCall) {
    EXPECT_EQ(project_add_call_edge(
        p, 0, "helper", 0, 7), 1);

    ASSERT_EQ(p->call_edge_count, 1);
    EXPECT_STREQ(p->call_edges[0].callee, "helper");
}

TEST_F(SourceScopeTest, DuplicateFunctionCall) {
    project_add_call_edge(p, 0, "helper", 0, 7);

    EXPECT_EQ(project_add_call_edge(
        p, 0, "helper", 0, 7), 1);

    EXPECT_EQ(p->call_edge_count, 1);
}

TEST_F(SourceScopeTest, FindFunctionByFile) {
    project_add_symbol(p, "helper", SYM_FUNCTION, 2, 1);
    EXPECT_EQ(project_find_function(p, "helper", 2), 0);
    EXPECT_EQ(project_find_function(p, "helper", 1), -1);
}

TEST_F(SourceScopeTest, SymbolTypeNames) {
    EXPECT_STREQ(symbol_type_name(SYM_FUNCTION), "FUNCTION");
    EXPECT_STREQ(symbol_type_name(SYM_VARIABLE), "VARIABLE");
    EXPECT_STREQ(symbol_type_name(SYM_MACRO), "MACRO");
    EXPECT_STREQ(symbol_type_name((SymbolType)99), "UNKNOWN");
}

TEST_F(SourceScopeTest, CleanupReleasesRoot) {
    project_add_symbol(p, "main", SYM_FUNCTION, 0, 1);
    project_cleanup(p);
    EXPECT_EQ(p->symbol_root, nullptr);
}

// SCANNER TEST CASES

TEST_F(SourceScopeTest, ScanExistingFile) {
    std::string path = "ss_test_existing.c";
    writeFile(path, "int main(void){return 0;}\n");

    EXPECT_EQ(scanner_add_file(p, path.c_str()), 1);
    ASSERT_EQ(p->file_count, 1);
    EXPECT_STREQ(p->files[0].path, path.c_str());

    remove(path.c_str());
}

TEST_F(SourceScopeTest, ScanMissingFile) {
    EXPECT_EQ(scanner_add_file(p, "ss_nonexistent_382719.c"), 0);
    EXPECT_EQ(p->file_count, 0);
}

TEST_F(SourceScopeTest, ScanEmptyFile) {
    std::string path = "ss_test_empty.h";
    writeFile(path, "");

    EXPECT_EQ(scanner_add_file(p, path.c_str()), 1);
    ASSERT_EQ(p->file_count, 1);
    EXPECT_EQ(p->files[0].length, 0u);

    remove(path.c_str());
}

TEST_F(SourceScopeTest, ScanDirectoryFiltersExtensions) {
    const char *dir = "ss_test_scan_dir";
    ASSERT_EQ(mkdir(dir, 0700), 0);

    writeFile(std::string(dir) + "/a.c", "int a;\n");
    writeFile(std::string(dir) + "/b.h", "#define B 1\n");
    writeFile(std::string(dir) + "/notes.txt", "ignored\n");

    scanner_scan_directory(p, dir, 0);
    EXPECT_EQ(p->file_count, 2);

    remove((std::string(dir) + "/a.c").c_str());
    remove((std::string(dir) + "/b.h").c_str());
    remove((std::string(dir) + "/notes.txt").c_str());
    rmdir(dir);
}

TEST_F(SourceScopeTest, ScannerStopsAtDepthLimit) {
    scanner_scan_directory(p, "ss_nonexistent_dir", 33);
    EXPECT_EQ(p->file_count, 0);
}

// PARSER TEST CASES

TEST_F(SourceScopeTest, ParseFunctionDefinition) {
    source("int calculate(void) { return 10; }\n");
    parser_analyze_file(p, 0);
    EXPECT_GE(project_find_function(p, "calculate", 0), 0);
}

TEST_F(SourceScopeTest, ParseMacroDefinition) {
    source("#define LIMIT 100\n");
    parser_analyze_file(p, 0);

    ASSERT_EQ(p->symbol_count, 1);
    EXPECT_STREQ(p->symbols[0].name, "LIMIT");
    EXPECT_EQ(p->symbols[0].type, SYM_MACRO);
}

TEST_F(SourceScopeTest, ParseIncludeDependency) {
    source("#include \"helper.h\"\n");
    parser_analyze_file(p, 0);

    ASSERT_EQ(p->file_edge_count, 1);
    EXPECT_STREQ(p->file_edges[0].to, "helper.h");
}

TEST_F(SourceScopeTest, ParseSimpleVariable) {
    source("int count = 5;\n");
    parser_analyze_file(p, 0);

    ASSERT_EQ(p->symbol_count, 1);
    EXPECT_STREQ(p->symbols[0].name, "count");
    EXPECT_EQ(p->symbols[0].type, SYM_VARIABLE);
}

TEST_F(SourceScopeTest, IgnoreFunctionInComment) {
    source("// int fake(void) { return 0; }\n"
           "int real(void) { return 1; }\n");

    parser_analyze_file(p, 0);

    EXPECT_EQ(project_find_function(p, "fake", 0), -1);
    EXPECT_GE(project_find_function(p, "real", 0), 0);
}

TEST_F(SourceScopeTest, ParseFunctionCall) {
    source("int helper(void) { return 1; }\n"
           "int main(void) { helper(); return 0; }\n");

    parser_analyze_file(p, 0);

    bool found = false;
    for (int i = 0; i < p->call_edge_count; i++) {
        if (!strcmp(p->call_edges[i].callee, "helper"))
            found = true;
    }

    EXPECT_TRUE(found);
}

// ANALYSIS TEST CASES

TEST_F(SourceScopeTest, AnalysisStatistics) {
    p->file_count = 1;
    project_add_symbol(p, "main", SYM_FUNCTION, 0, 1);

    testing::internal::CaptureStdout();
    analysis_print_statistics(p);
    std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Functions: 1"), std::string::npos);
}

TEST_F(SourceScopeTest, AnalysisSymbolListing) {
    source("int main(void){}\n", "main.c");
    project_add_symbol(p, "main", SYM_FUNCTION, 0, 1);

    testing::internal::CaptureStdout();
    analysis_print_symbols(p);
    std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("main"), std::string::npos);
}

TEST_F(SourceScopeTest, AnalysisDependencyListing) {
    source("#include \"util.h\"\n", "main.c");
    project_add_file_edge(p, "main.c", "util.h", "INCLUDE", 1);

    testing::internal::CaptureStdout();
    analysis_print_dependencies(p);
    std::string output =
        testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("util.h"), std::string::npos);
}

TEST_F(SourceScopeTest, AnalysisReportCreation) {
    source("int main(void){}\n", "main.c");
    project_add_symbol(p, "main", SYM_FUNCTION, 0, 1);

    const char *path = "ss_test_report.txt";
    analysis_write_report(p, path);

    std::string report = readFile(path);

    EXPECT_NE(report.find("SourceScope Analysis Report"),
              std::string::npos);
    EXPECT_NE(report.find("main"), std::string::npos);

    remove(path);
}

TEST_F(SourceScopeTest, AnalysisSearchSymbol) {
    source("int main(void){}\n", "main.c");
    project_add_symbol(p, "main", SYM_FUNCTION, 0, 1);

    std::string output =
        withInput("main\n", analysis_search_symbols, p);

    EXPECT_NE(output.find("BST match: main"),
              std::string::npos);
}

TEST_F(SourceScopeTest, AnalysisChangeImpact) {
    source("#include \"util.h\"\n", "main.c");
    source("int helper(void){}\n", "util.h");

    project_add_file_edge(p, "main.c", "util.h", "INCLUDE", 1);

    std::string output =
        withInput("util.h\n", analysis_impact, p);

    EXPECT_NE(output.find("main.c"), std::string::npos);
    EXPECT_NE(output.find("util.h"), std::string::npos);
}
