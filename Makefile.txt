
CC = gcc
CXX = g++
CPPFLAGS = -I.
CFLAGS = -std=c11 -g -O0
CXXFLAGS = -std=c++17 -g -O0

APP = sourcescope
TEST_APP = build/sourcescope_tests

C_SOURCES = main.c scanner.c parser.c bst.c project.c analysis.c logger.c
CORE_SOURCES = scanner.c parser.c bst.c project.c analysis.c logger.c
TEST_SOURCE = sourcescope_unit_tests.cpp

BUILD_DIR = build
COVERAGE_DIR = $(BUILD_DIR)/coverage

APP_OBJECTS = $(C_SOURCES:%.c=$(BUILD_DIR)/app/%.o)
TEST_OBJECTS = $(CORE_SOURCES:%.c=$(BUILD_DIR)/test/%.o)
COVERAGE_OBJECTS = $(CORE_SOURCES:%.c=$(COVERAGE_DIR)/%.o)

.PHONY: all app test run clean coverage valgrind helgrind cppcheck misra sanitize help

all: app

app: $(APP)

$(APP): $(APP_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/app/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_APP): $(TEST_OBJECTS) $(TEST_SOURCE)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TEST_SOURCE) $(TEST_OBJECTS) -o $@ -lgtest -lgtest_main -pthread

test: $(TEST_APP)
	./$(TEST_APP)

run: app
	./$(APP) ./sample_project

$(COVERAGE_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) --coverage -c $< -o $@

$(BUILD_DIR)/sourcescope_tests_coverage: $(COVERAGE_OBJECTS) $(TEST_SOURCE)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) --coverage $(TEST_SOURCE) $(COVERAGE_OBJECTS) -o $@ -lgtest -lgtest_main -pthread

coverage: $(BUILD_DIR)/sourcescope_tests_coverage
	./$(BUILD_DIR)/sourcescope_tests_coverage
	gcov -b -c -o $(COVERAGE_DIR) $(CORE_SOURCES)
	@if command -v gcovr >/dev/null 2>&1; then \
		gcovr --root . --object-directory $(COVERAGE_DIR) --txt --html --html-details -o $(BUILD_DIR)/coverage.html; \
		echo "HTML coverage report: $(BUILD_DIR)/coverage.html"; \
	else \
		echo "gcovr is not installed; gcov summary files were generated."; \
	fi

valgrind: $(TEST_APP)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./$(TEST_APP)

helgrind: $(TEST_APP)
	valgrind --tool=helgrind --error-exitcode=1 ./$(TEST_APP)

cppcheck:
	cppcheck --enable=warning,style,performance,portability --inline-suppr --error-exitcode=1 --language=c --std=c11 -I. $(C_SOURCES)

misra:
	cppcheck --addon=misra --enable=warning,style,performance,portability --inline-suppr --error-exitcode=1 --language=c --std=c11 -I. $(C_SOURCES)

sanitize:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -std=c11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(C_SOURCES) -o $(BUILD_DIR)/$(APP)_sanitize
	ASAN_OPTIONS=detect_leaks=1 ./$(BUILD_DIR)/$(APP)_sanitize ./sample_project

clean:
	rm -rf $(BUILD_DIR) $(APP) *.gcda *.gcno *.gcov

help:
	@echo "Available targets:"
	@echo "  make app       - Compile project"
	@echo "  make test      - Run Google Test"
	@echo "  make run       - Run project"
	@echo "  make coverage  - Run gcov and gcovr"
	@echo "  make valgrind  - Run Valgrind"
	@echo "  make helgrind  - Run Helgrind"
	@echo "  make cppcheck  - Run Cppcheck"
	@echo "  make misra     - Run MISRA analysis"
	@echo "  make sanitize  - Run sanitizers"
	@echo "  make clean     - Clean generated files"
