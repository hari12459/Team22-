# SourceScope — Modular C Project

## File layout
- `main.c`: program entry point and menu
- `common.h`: shared data structures
- `scanner.c` / `scanner.h`: recursive directory scanning and file reading
- `parser.c` / `parser.h`: basic source parsing, include/macro detection, function definitions/calls, simple variable detection
- `bst.c` / `bst.h`: BST symbol index
- `project.c` / `project.h`: project data, symbol/edge insertion, cleanup
- `analysis.c` / `analysis.h`: statistics, symbol search, dependency listing, impact analysis, report output

## Compile (Linux or MSYS2)
```sh
gcc -std=c11 -Wall -Wextra -pedantic main.c scanner.c parser.c bst.c project.c analysis.c -o sourcescope
```

## Run
```sh
./sourcescope ./sample_project
```
Windows: `sourcescope.exe ./sample_project`

The analyzer is heuristic rather than a complete C parser. Complex declarations, macros, preprocessor branches, and cross-file function resolution can be inaccurate. Add automated tests, cache invalidation, Graphviz output, Cppcheck, and coverage measurement to complete the wider POC specification.
