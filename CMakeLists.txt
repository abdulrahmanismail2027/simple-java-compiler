cmake_minimum_required(VERSION 3.25)

project(simple_java_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================
# Core components
# ============================================================

add_library(core
src/core/SymbolTable.cpp
)

# ============================================================
# Input/output components
# ============================================================

add_library(io
src/io/RuleParser.cpp
)

# ============================================================
# Automata and regular expression components
# ============================================================

add_library(automata
src/automata/DFATransitionTable.cpp
src/automata/NFATransitionTable.cpp
src/automata/NFA.cpp
src/automata/DFA.cpp
src/automata/FA.cpp
src/automata/ThompsonConstructor.cpp
src/regex/RegexParser.cpp
        src/regex/RegexPreprocessor.cpp
        include/regex/RegexPreprocessor.h
        include/automata/LexerNFAFactory.h
        src/automata/LexerNFAFactory.cpp
        src/lexer/NFALexer.cpp
        include/lexer/NFALexer.h
)

# ============================================================
# Main lexer executable
# ============================================================

add_executable(lexer_main
#src/lexer/main.cpp
src/lexer/NFABuildingMain.cpp
)

target_link_libraries(lexer_main
PRIVATE
core
io
automata
)

# ============================================================
# GoogleTest setup
# ============================================================

include(FetchContent)

FetchContent_Declare(
googletest
URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

# ============================================================
# Unit tests
# ============================================================

add_executable(unit_tests
tests/io/test_RuleParser.cpp
tests/regex/test_RegexParser.cpp
tests/automata/test_ThompsonConstruction.cpp
)

target_link_libraries(unit_tests
PRIVATE
core
io
automata
GTest::gtest_main
)

# Automatically discover GoogleTest cases
include(GoogleTest)

gtest_discover_tests(unit_tests)
