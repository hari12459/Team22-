TEST(ParserTest, ExtractMacroAndIncludeDependency)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    const char source[] =
        "#define LIMIT 10\n"
        "#include \"utils.h\"\n"
        "int run(void) { return LIMIT; }\n";

    p->file_count = 1;
    strcpy(p->files[0].path, "test.c");
    p->files[0].source = static_cast<char*>(malloc(sizeof(source)));
    ASSERT_NE(p->files[0].source, nullptr);
    memcpy(p->files[0].source, source, sizeof(source));
    p->files[0].length = strlen(source);

    parser_analyze_file(p, 0);

    bool macro_found = false;
    bool function_found = false;
    for (int i = 0; i < p->symbol_count; ++i) {
        if (strcmp(p->symbols[i].name, "LIMIT") == 0 &&
            p->symbols[i].type == SYM_MACRO) macro_found = true;
        if (strcmp(p->symbols[i].name, "run") == 0 &&
            p->symbols[i].type == SYM_FUNCTION) function_found = true;
    }

    EXPECT_TRUE(macro_found);
    EXPECT_TRUE(function_found);
    ASSERT_EQ(p->file_edge_count, 1);
    EXPECT_STREQ(p->file_edges[0].to, "utils.h");

    destroy_project(p);
}

TEST(ParserTest, RecordsFunctionCall)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    const char source[] =
        "int add(void) { return 1; }\n"
        "int main(void) { add(); return 0; }\n";

    p->file_count = 1;
    strcpy(p->files[0].path, "test.c");
    p->files[0].source = static_cast<char*>(malloc(sizeof(source)));
    ASSERT_NE(p->files[0].source, nullptr);
    memcpy(p->files[0].source, source, sizeof(source));
    p->files[0].length = strlen(source);

    parser_analyze_file(p, 0);

    bool call_found = false;
    for (int i = 0; i < p->call_edge_count; ++i) {
        if (strcmp(p->call_edges[i].callee, "add") == 0) call_found = true;
    }
    EXPECT_TRUE(call_found);

    destroy_project(p);
}

TEST(ParserTest, IgnoresFunctionTextInsideComment)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    const char source[] =
        "/* int fake(void) { return 0; } */\n"
        "int real(void) { return 1; }\n";

    p->file_count = 1;
    strcpy(p->files[0].path, "test.c");
    p->files[0].source = static_cast<char*>(malloc(sizeof(source)));
    ASSERT_NE(p->files[0].source, nullptr);
    memcpy(p->files[0].source, source, sizeof(source));
    p->files[0].length = strlen(source);

    parser_analyze_file(p, 0);

    EXPECT_EQ(project_find_function(p, "fake", 0), -1);
    EXPECT_GE(project_find_function(p, "real", 0), 0);

    destroy_project(p);
}

// ============================ ANALYSIS TESTS ==========================

TEST(AnalysisTest, PrintStatisticsSymbolsAndDependencies)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    strcpy(p->files[0].path, "main.c");
    p->file_count = 1;
    project_add_symbol(p, "main", SYM_FUNCTION, 0, 1);
    project_add_file_edge(p, "main.c", "utils.h", "INCLUDE", 1);
    project_add_call_edge(p, 0, "printf", 0, 2);

    EXPECT_NO_THROW(analysis_print_statistics(p));
    EXPECT_NO_THROW(analysis_print_symbols(p));
    EXPECT_NO_THROW(analysis_print_dependencies(p));

    destroy_project(p);
}

TEST(AnalysisTest, SearchSymbolsWithMatchingName)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    strcpy(p->files[0].path, "main.c");
    p->file_count = 1;
    project_add_symbol(p, "calculate", SYM_FUNCTION, 0, 1);

    InputRedirect input("calculate\n");
    EXPECT_NO_THROW(analysis_search_symbols(p));

    destroy_project(p);
}

TEST(AnalysisTest, SearchSymbolsWithMissingName)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    strcpy(p->files[0].path, "main.c");
    p->file_count = 1;
    project_add_symbol(p, "calculate", SYM_FUNCTION, 0, 1);

    InputRedirect input("not_found\n");
    EXPECT_NO_THROW(analysis_search_symbols(p));

    destroy_project(p);
}

TEST(AnalysisTest, ImpactAnalysisForMatchingFile)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    p->file_count = 2;
    strcpy(p->files[0].path, "utils.h");
    strcpy(p->files[1].path, "main.c");
    project_add_file_edge(p, "main.c", "utils.h", "INCLUDE", 1);

    InputRedirect input("utils.h\n");
    EXPECT_NO_THROW(analysis_impact(p));

    destroy_project(p);
}

TEST(AnalysisTest, ImpactAnalysisForUnknownFile)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    p->file_count = 1;
    strcpy(p->files[0].path, "main.c");
