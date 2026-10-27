#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <fcntl.h>

extern "C" {
#include "common.h"
#include "analysis.h"
#include "bst.h"
#include "parser.h"
#include "project.h"
#include "scanner.h"
}

// ============================ HELPERS =================================

static Project* create_project()
{
    return static_cast<Project*>(calloc(1, sizeof(Project)));
}

static void destroy_project(Project* p)
{
    if (p != nullptr) {
        project_cleanup(p);
        free(p);
    }
}

// Redirect stdin so functions that use scanf() can be tested.
class InputRedirect {
public:
    explicit InputRedirect(const char* input) : saved_stdin_(-1), file_(nullptr)
    {
        fflush(stdout);
        file_ = tmpfile();
        if (file_ == nullptr) return;

        size_t length = strlen(input);
        if (fwrite(input, 1, length, file_) != length) return;
        rewind(file_);

        saved_stdin_ = dup(STDIN_FILENO);
        if (saved_stdin_ == -1) return;

        if (dup2(fileno(file_), STDIN_FILENO) == -1) {
            close(saved_stdin_);
            saved_stdin_ = -1;
            return;
        }

        clearerr(stdin);
    }

    ~InputRedirect()
    {
        if (saved_stdin_ != -1) {
            dup2(saved_stdin_, STDIN_FILENO);
            close(saved_stdin_);
            clearerr(stdin);
        }
        if (file_ != nullptr) fclose(file_);
    }

    InputRedirect(const InputRedirect&) = delete;
    InputRedirect& operator=(const InputRedirect&) = delete;

private:
    int saved_stdin_;
    FILE* file_;
};

// ============================ BST TESTS ===============================

TEST(BSTTest, InsertAndFindSymbol)
{
    Symbol symbol = {};
    strcpy(symbol.name, "calculate");

    BSTNode* root = nullptr;
    bst_insert(&root, &symbol);

    EXPECT_NE(bst_find(root, "calculate"), nullptr);
    bst_free(root);
}

TEST(BSTTest, FindMissingSymbol)
{
    Symbol symbol = {};
    strcpy(symbol.name, "calculate");

    BSTNode* root = nullptr;
