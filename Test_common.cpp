
    bst_free(root);
}

TEST(BSTTest, InsertMultipleSymbols)
{
    BSTNode* root = nullptr;
    root = bst_insert(root, "main", 0);
    root = bst_insert(root, "add", 1);
    root = bst_insert(root, "display", 2);

    EXPECT_EQ(bst_find(root, "main"), 0);
    EXPECT_EQ(bst_find(root, "add"), 1);
    EXPECT_EQ(bst_find(root, "display"), 2);

    bst_free(root);
}

// PROJECT MODULE TESTS

TEST(ProjectTest, AddFunctionSymbol)
{
    Project *p = (Project*)calloc(1, sizeof(Project));
    ASSERT_NE(p, nullptr);
    int index = project_add_symbol(
        p, "main", SYM_FUNCTION, 0, 1);

    EXPECT_EQ(index, 0);
    EXPECT_EQ(p->symbol_count, 1);
    EXPECT_EQ(p->symbols[0].type, SYM_FUNCTION);

    project_cleanup(p);
    free(p);
}

TEST(ProjectTest, AddFileDependency)
{
    Project *p = (Project*)calloc(1, sizeof(Project));
    ASSERT_NE(p, nullptr);

    EXPECT_EQ(project_add_file_edge(
        p, "main.c", "utils.h", "INCLUDE", 1), 1);

    EXPECT_EQ(p->file_edge_count, 1);

    EXPECT_STREQ(p->file_edges[0].from, "main.c");
    EXPECT_STREQ(p->file_edges[0].to, "utils.h");
    project_cleanup(p);
    free(p);
}

TEST(ProjectTest, AddFunctionCall)
{
    Project *p = (Project*)calloc(1, sizeof(Project));
    ASSERT_NE(p, nullptr);

    EXPECT_EQ(project_add_call_edge(
        p, 0, "calculate", 0, 5), 1);

    EXPECT_EQ(p->call_edge_count, 1);
    EXPECT_EQ(p->call_edges[0].caller_symbol, 0);
    EXPECT_STREQ(p->call_edges[0].callee, "calculate");
    project_cleanup(p);
    free(p);
}

// PARSER TEST

TEST(ParserTest, ExtractFunctionDefinition)
{
    Project *p = (Project*)calloc(1, sizeof(Project));
    ASSERT_NE(p, nullptr);

    p->file_count = 1;
    strcpy(p->files[0].path, "test.c");
    const char source[] = "int calculate(void) { return 10; }";
    p->files[0].source = (char*)malloc(sizeof(source));
    ASSERT_NE(p->files[0].source, nullptr);
    memcpy(p->files[0].source, source, sizeof(source));
    p->files[0].length = strlen(source);

    parser_analyze_file(p, 0);

    EXPECT_EQ(p->symbol_count, 1);
    EXPECT_STREQ(p->symbols[0].name, "calculate");
    EXPECT_EQ(p->symbols[0].type, SYM_FUNCTION);

    project_cleanup(p);
    free(p);
}

// SCANNER TEST

TEST(ScannerTest, ReadExistingSourceFile)
{
    Project *p = (Project*)calloc(1, sizeof(Project));
    ASSERT_NE(p, nullptr);

    const char *path = "test_input.c";
    FILE *fp = fopen(path, "w");
    ASSERT_NE(fp, nullptr);
    fputs("int main(void) { return 0; }\n", fp);
    fclose(fp);
    EXPECT_EQ(scanner_add_file(p, path), 1);
    EXPECT_EQ(p->file_count, 1);
    EXPECT_STREQ(p->files[0].path, "test_input.c");

    project_cleanup(p);
    free(p);
    remove("test_input.c");
}
