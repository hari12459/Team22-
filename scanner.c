#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "scanner.h"
static int ends_with(const char *s, const char *suffix)
{
    size_t a = strlen(s), b = strlen(suffix);
    return a >= b && !strcmp(s + a - b, suffix);
}
static void join_path(char *out, size_t cap, const char *dir, const char *name)
{
    size_t n = strlen(dir);
    const char *sep = n && dir[n-1] != '/' && dir[n-1] != '\\' ? "/" : "";
    snprintf(out, cap, "%s%s%s", dir, sep, name);
}
int scanner_add_file(Project *p, const char *path)
{
    FILE *fp; long size; char *data; size_t got; int index;
    if (p->file_count >= SS_MAX_FILES) return 0;
    fp = fopen(path, "rb");
    if (!fp) { fprintf(stderr, "Warning: cannot open %s: %s\n", path, strerror(errno)); return 0; }
    if (fseek(fp, 0, SEEK_END) != 0 || (size = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp); return 0;
    }
    data = (char *)malloc((size_t)size + 1);
    if (!data) { fclose(fp); return 0; }
    got = fread(data, 1, (size_t)size, fp); fclose(fp);
    if (got != (size_t)size) { free(data); return 0; }
    data[size] = '\0'; index = p->file_count++;
    snprintf(p->files[index].path, sizeof(p->files[index].path), "%s", path);
    p->files[index].source = data; p->files[index].length = (size_t)size; return 1;
}
void scanner_scan_directory(Project *p, const char *dir_name, int depth)
{
    DIR *dir; struct dirent *entry;
    if (depth > 32) return;
    dir = opendir(dir_name);
    if (!dir) { fprintf(stderr, "Warning: cannot open directory %s\n", dir_name); return; }
    while ((entry = readdir(dir)) != NULL) {
        char path[SS_MAX_PATH]; struct stat st;
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        join_path(path, sizeof(path), dir_name, entry->d_name);
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (strcmp(entry->d_name, ".git") && strcmp(entry->d_name, "build") &&
                strcmp(entry->d_name, "out")) scanner_scan_directory(p, path, depth + 1);
        } else if (S_ISREG(st.st_mode) &&
                   (ends_with(entry->d_name, ".c") || ends_with(entry->d_name, ".h"))) {
            (void)scanner_add_file(p, path);
        }
    }
    closedir(dir);
}
