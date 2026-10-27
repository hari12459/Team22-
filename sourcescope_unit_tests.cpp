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

    p->file_count = 1;
    strcpy(p->files[0].path, "main.c");
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

    p->file_count = 1;
    strcpy(p->files[0].path, "main.c");
    project_add_symbol(p, "calculate", SYM_FUNCTION, 0, 1);

    {
        InputRedirect input("calculate\n");
        EXPECT_NO_THROW(analysis_search_symbols(p));
    }

    destroy_project(p);
}

TEST(AnalysisTest, SearchSymbolsWithMissingName)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    p->file_count = 1;
    strcpy(p->files[0].path, "main.c");
    project_add_symbol(p, "calculate", SYM_FUNCTION, 0, 1);

    {
        InputRedirect input("not_found\n");
        EXPECT_NO_THROW(analysis_search_symbols(p));
    }

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

    {
        InputRedirect input("utils.h\n");
        EXPECT_NO_THROW(analysis_impact(p));
    }

    destroy_project(p);
}

TEST(AnalysisTest, ImpactAnalysisForUnknownFile)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    p->file_count = 1;
    strcpy(p->files[0].path, "main.c");

    {
        InputRedirect input("unknown.c\n");
        EXPECT_NO_THROW(analysis_impact(p));
    }

    destroy_project(p);
}

TEST(AnalysisTest, WriteReportToFile)
{
    Project* p = create_project();
    ASSERT_NE(p, nullptr);

    p->file_count = 1;
    strcpy(p->files[0].path, "main.c");
    project_add_symbol(p, "main", SYM_FUNCTION, 0, 1);

    char report_path[] = "/tmp/sourcescope_report_XXXXXX";
    int fd = mkstemp(report_path);
    ASSERT_NE(fd, -1);
    close(fd);

    analysis_write_report(p, report_path);

    FILE* report = fopen(report_path, "r");
    ASSERT_NE(report, nullptr);
    fclose(report);
    remove(report_path);

    destroy_project(p);
}
