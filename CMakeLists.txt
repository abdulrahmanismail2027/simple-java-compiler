cmake_minimum_required(VERSION 4.3)
project(simple_java_compiler)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_library(core
        src/core/SymbolTable.cpp
)

add_library(io
        src/io/RuleParser.cpp
)

add_library(automata
        src/automata/DFATransitionTable.cpp
        src/automata/NFATransitionTable.cpp
        src/automata/NFA.cpp
        src/automata/DFA.cpp
        src/automata/FA.cpp
)

add_executable(lexer_main src/lexer/main.cpp)
target_link_libraries(lexer_main PRIVATE core io automata)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(unit_tests
        tests/io/test_RuleParser.cpp
)

target_link_libraries(unit_tests PRIVATE core io automata gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
