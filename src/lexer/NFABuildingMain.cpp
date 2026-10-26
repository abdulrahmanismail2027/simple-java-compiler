#include <algorithm>
#include <cctype>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "io/RuleParser.h"
#include "regex/RegexPreprocessor.h"
#include "automata/LexerNFAFactory.h"
#include "automata/NFA.h"
#include "automata/NFATransitionTable.h"
#include "lexer/NFALexer.h"

// ============================================================
// TEST DATA STRUCTURES
// ============================================================

using ExpectedToken = std::pair<std::string, std::string>;
// ExpectedToken = {token class, lexeme}

struct ExpectedError {
    char character;
    std::size_t line;
    std::size_t column;
};

struct TestCase {
    std::string name;
    std::string source;
    std::vector<ExpectedToken> expectedTokens;
    std::vector<ExpectedError> expectedErrors;
};

static ExpectedToken T(
    const std::string& tokenClass,
    const std::string& lexeme
) {
    return {tokenClass, lexeme};
}

// ============================================================
// DISPLAY HELPERS
// ============================================================

static std::string displayCharacter(char character) {
    switch (character) {
        case '\n': return "'\\n'";
        case '\t': return "'\\t'";
        case '\r': return "'\\r'";
        case '\0': return "'\\0'";
        case '\\': return "'\\\\'";
        case '\'': return "'\\''";

        default:
            if (std::isprint(static_cast<unsigned char>(character))) {
                return std::string("'") + character + "'";
            }

            return "ASCII(" +
                   std::to_string(
                       static_cast<unsigned char>(character)
                   ) +
                   ")";
    }
}

static void printParsedRules(const ParsedRules& rules) {
    std::cout
        << "\n============================================================\n"
        << "1. PARSED LEXICAL RULES\n"
        << "============================================================\n";

    std::cout << "\nRegular definitions:\n";

    for (const auto& [name, regex] : rules.regular_definitions) {
        std::cout << "  " << name << " = " << regex << '\n';
    }

    std::cout << "\nKeywords:\n";

    for (const auto& keyword : rules.keywords) {
        std::cout << "  " << keyword << '\n';
    }

    std::cout << "\nPunctuation:\n";

    for (const auto& punctuation : rules.punctuations) {
        std::cout << "  " << punctuation << '\n';
    }

    std::cout << "\nRegular expressions:\n";

    for (const auto& [name, regex] : rules.regular_expressions) {
        std::cout << "  " << name << " : " << regex << '\n';
    }
}

static void printExpandedRegexes(const ParsedRules& rules) {
    RegexPreprocessor preprocessor(rules);

    std::cout
        << "\n============================================================\n"
        << "2. EXPANDED REGULAR EXPRESSIONS\n"
        << "============================================================\n";

    for (const auto& [name, regex] : rules.regular_expressions) {
        std::cout
            << "\n" << name << " :\n"
            << preprocessor.expand(regex)
            << '\n';
    }
}

static void printNFAStatistics(const NFA& nfa) {
    const auto& transitionTable = nfa.getTable().getTable();

    std::size_t transitionCount = 0;

    for (const auto& [fromState, transitions] : transitionTable) {
        (void)fromState;

        for (const auto& [symbol, destinations] : transitions) {
            (void)symbol;
            transitionCount += destinations.size();
        }
    }

    std::cout
        << "\n============================================================\n"
        << "3. COMBINED LEXER NFA\n"
        << "============================================================\n";

    std::cout << "Start state: "
              << nfa.getStartState() << '\n';

    std::cout << "States tracked: "
              << nfa.getTable().getStates().size() << '\n';

    std::cout << "Transitions: "
              << transitionCount << '\n';

    std::cout << "Accepting states: "
              << nfa.getAcceptStates().size() << '\n';

    std::cout
        << "\nThe combined NFA will now be used to tokenize input.\n";
}

static void printTokens(const std::vector<Token>& tokens) {
    std::cout
        << "\nRecognized tokens:\n"
        << "-----+------------------+----------------------+"
        << "-----------+----------\n"
        << "  #  | Token class      | Lexeme               |"
        << " Line:Col | Priority\n"
        << "-----+------------------+----------------------+"
        << "-----------+----------\n";

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const Token& token = tokens[i];

        std::cout
            << std::right << std::setw(4) << (i + 1)
            << " | "
            << std::left << std::setw(16) << token.tokenClass
            << " | "
            << std::setw(20) << std::quoted(token.lexeme)
            << " | "
            << std::right << std::setw(4) << token.line
            << ':' << std::left << std::setw(4) << token.column
            << " | "
            << std::right << std::setw(3) << token.priority
            << '\n';
    }

    std::cout
        << "-----+------------------+----------------------+"
        << "-----------+----------\n";

    std::cout << "Total tokens: " << tokens.size() << '\n';
}

static void printErrors(const std::vector<LexicalError>& errors) {
    std::cout << "\nLexical errors:\n";

    if (errors.empty()) {
        std::cout << "  None\n";
        return;
    }

    for (const auto& error : errors) {
        std::cout
            << "  Unrecognized character "
            << displayCharacter(error.character)
            << " at line " << error.line
            << ", column " << error.column
            << " (zero-based offset " << error.offset << ")\n";
    }

    std::cout << "Total errors: " << errors.size() << '\n';
}

// ============================================================
// TEST RUNNER
// ============================================================

static bool runTest(
    const NFALexer& lexer,
    const TestCase& test
) {
    std::cout
        << "\n\n############################################################\n"
        << "TEST: " << test.name << '\n'
        << "############################################################\n";

    std::cout << "\nInput source:\n"
              << "--------------------\n";

    if (test.source.empty()) {
        std::cout << "<empty input>\n";
    } else {
        std::cout << test.source;

        if (test.source.back() != '\n') {
            std::cout << '\n';
        }
    }

    std::cout << "--------------------\n";

    const TokenizationResult result = lexer.tokenize(test.source);

    printTokens(result.tokens);
    printErrors(result.errors);

    // Compare token count and complete ordered token stream.
    bool tokensMatch =
        result.tokens.size() == test.expectedTokens.size();

    const std::size_t comparableCount =
        std::min(result.tokens.size(), test.expectedTokens.size());

    for (std::size_t i = 0; i < comparableCount; ++i) {
        const Token& actual = result.tokens[i];
        const ExpectedToken& expected = test.expectedTokens[i];

        if (actual.tokenClass != expected.first ||
            actual.lexeme != expected.second) {
            tokensMatch = false;

            std::cout
                << "\nTOKEN MISMATCH at token " << (i + 1)
                << "\n  Expected: class=" << expected.first
                << ", lexeme=" << std::quoted(expected.second)
                << "\n  Actual:   class=" << actual.tokenClass
                << ", lexeme=" << std::quoted(actual.lexeme)
                << '\n';
        }
    }

    if (result.tokens.size() != test.expectedTokens.size()) {
        std::cout
            << "\nTOKEN COUNT MISMATCH"
            << "\n  Expected: " << test.expectedTokens.size()
            << "\n  Actual:   " << result.tokens.size()
            << '\n';

        if (result.tokens.size() < test.expectedTokens.size()) {
            for (std::size_t i = result.tokens.size();
                 i < test.expectedTokens.size();
                 ++i) {
                std::cout
                    << "  Missing expected token " << (i + 1)
                    << ": class=" << test.expectedTokens[i].first
                    << ", lexeme="
                    << std::quoted(test.expectedTokens[i].second)
                    << '\n';
            }
        } else {
            for (std::size_t i = test.expectedTokens.size();
                 i < result.tokens.size();
                 ++i) {
                std::cout
                    << "  Unexpected token " << (i + 1)
                    << ": class=" << result.tokens[i].tokenClass
                    << ", lexeme="
                    << std::quoted(result.tokens[i].lexeme)
                    << '\n';
            }
        }
    }

    // Compare the error characters and their source locations.
    bool errorsMatch =
        result.errors.size() == test.expectedErrors.size();

    const std::size_t comparableErrors =
        std::min(result.errors.size(), test.expectedErrors.size());

    for (std::size_t i = 0; i < comparableErrors; ++i) {
        const LexicalError& actual = result.errors[i];
        const ExpectedError& expected = test.expectedErrors[i];

        if (actual.character != expected.character ||
            actual.line != expected.line ||
            actual.column != expected.column) {
            errorsMatch = false;

            std::cout
                << "\nERROR MISMATCH at error " << (i + 1)
                << "\n  Expected: character="
                << displayCharacter(expected.character)
                << ", line=" << expected.line
                << ", column=" << expected.column
                << "\n  Actual:   character="
                << displayCharacter(actual.character)
                << ", line=" << actual.line
                << ", column=" << actual.column
                << '\n';
        }
    }

    if (result.errors.size() != test.expectedErrors.size()) {
        std::cout
            << "\nERROR COUNT MISMATCH"
            << "\n  Expected: " << test.expectedErrors.size()
            << "\n  Actual:   " << result.errors.size()
            << '\n';

        if (result.errors.size() > test.expectedErrors.size()) {
            for (std::size_t i = test.expectedErrors.size();
                 i < result.errors.size();
                 ++i) {
                const LexicalError& error = result.errors[i];

                std::cout
                    << "  Unexpected error: "
                    << displayCharacter(error.character)
                    << " at line " << error.line
                    << ", column " << error.column
                    << '\n';
            }
        }
    }

    const bool passed = tokensMatch && errorsMatch;

    std::cout
        << "\nToken sequence: "
        << (tokensMatch ? "PASS" : "FAIL")
        << "\nError handling: "
        << (errorsMatch ? "PASS" : "FAIL")
        << "\nRESULT: "
        << (passed ? "PASS" : "FAIL")
        << '\n';

    return passed;
}

// ============================================================
// MAIN
// ============================================================

int main() {
    try {
        // 1. Parse lexical rules.
        RuleParser ruleParser("examples/lexical_rules.txt");
        const ParsedRules rules = ruleParser.parse();

        printParsedRules(rules);
        printExpandedRegexes(rules);

        // 2. Build the combined lexer NFA.
        const NFA lexerNFA = LexerNFAFactory::build(rules);

        printNFAStatistics(lexerNFA);

        // The NFA must outlive the lexer because the lexer holds
        // a reference to it.
        const NFALexer lexer(lexerNFA);

        std::vector<TestCase> tests;

        // ----------------------------------------------------
        // TEST 1: A complete Java-like program.
        // ----------------------------------------------------

        const std::string program =
            "boolean flag = 1;\n"
            "int count = 123;\n"
            "float rate = 12.34;\n"
            "if (count >= 10) {\n"
            "    count = count + 1;\n"
            "} else {\n"
            "    count = count - 1;\n"
            "}\n"
            "while (count != 0) {\n"
            "    count = count * 2 / 2;\n"
            "}\n";

        tests.push_back({
            "1. Full Java-like program",
            program,
            {
                T("KEYWORD", "boolean"),
                T("id", "flag"),
                T("assign", "="),
                T("num", "1"),
                T("PUNCTUATION", ";"),

                T("KEYWORD", "int"),
                T("id", "count"),
                T("assign", "="),
                T("num", "123"),
                T("PUNCTUATION", ";"),

                T("KEYWORD", "float"),
                T("id", "rate"),
                T("assign", "="),
                T("num", "12.34"),
                T("PUNCTUATION", ";"),

                T("KEYWORD", "if"),
                T("PUNCTUATION", "("),
                T("id", "count"),
                T("relop", ">="),
                T("num", "10"),
                T("PUNCTUATION", ")"),
                T("PUNCTUATION", "{"),

                T("id", "count"),
                T("assign", "="),
                T("id", "count"),
                T("addop", "+"),
                T("num", "1"),
                T("PUNCTUATION", ";"),

                T("PUNCTUATION", "}"),
                T("KEYWORD", "else"),
                T("PUNCTUATION", "{"),

                T("id", "count"),
                T("assign", "="),
                T("id", "count"),
                T("addop", "-"),
                T("num", "1"),
                T("PUNCTUATION", ";"),

                T("PUNCTUATION", "}"),

                T("KEYWORD", "while"),
                T("PUNCTUATION", "("),
                T("id", "count"),
                T("relop", "!="),
                T("num", "0"),
                T("PUNCTUATION", ")"),
                T("PUNCTUATION", "{"),

                T("id", "count"),
                T("assign", "="),
                T("id", "count"),
                T("mulop", "*"),
                T("num", "2"),
                T("mulop", "/"),
                T("num", "2"),
                T("PUNCTUATION", ";"),

                T("PUNCTUATION", "}")
            },
            {}
        });

        // ----------------------------------------------------
        // TEST 2: Maximal munch for overlapping operators.
        // ----------------------------------------------------

        tests.push_back({
            "2. Maximal munch and operator recognition",
            "= == > >= < <= != + - * /",
            {
                T("assign", "="),
                T("relop", "=="),
                T("relop", ">"),
                T("relop", ">="),
                T("relop", "<"),
                T("relop", "<="),
                T("relop", "!="),
                T("addop", "+"),
                T("addop", "-"),
                T("mulop", "*"),
                T("mulop", "/")
            },
            {}
        });

        // ----------------------------------------------------
        // TEST 3: Keyword versus identifier priority.
        // ----------------------------------------------------

        tests.push_back({
            "3. Keyword versus identifier priority",
            "if ifx int int2 while while2 boolean booleanx",
            {
                T("KEYWORD", "if"),
                T("id", "ifx"),
                T("KEYWORD", "int"),
                T("id", "int2"),
                T("KEYWORD", "while"),
                T("id", "while2"),
                T("KEYWORD", "boolean"),
                T("id", "booleanx")
            },
            {}
        });

        // ----------------------------------------------------
        // TEST 4: Numeric literals.
        //
        // This reflects the currently configured num rule.
        // ----------------------------------------------------

        tests.push_back({
            "4. Numeric literals",
            "0 123 12.34 12.34E5",
            {
                T("num", "0"),
                T("num", "123"),
                T("num", "12.34"),
                T("num", "12.34E5")
            },
            {}
        });

        // ----------------------------------------------------
        // TEST 5: Punctuation, especially parentheses.
        // ----------------------------------------------------

        tests.push_back({
            "5. Punctuation and escaped parentheses",
            "; , ( ) { }",
            {
                T("PUNCTUATION", ";"),
                T("PUNCTUATION", ","),
                T("PUNCTUATION", "("),
                T("PUNCTUATION", ")"),
                T("PUNCTUATION", "{"),
                T("PUNCTUATION", "}")
            },
            {}
        });

        // ----------------------------------------------------
        // TEST 6: Adjacent tokens without spaces.
        // ----------------------------------------------------

        tests.push_back({
            "6. Adjacent numbers and identifiers",
            "123abc 12.34E5x",
            {
                T("num", "123"),
                T("id", "abc"),
                T("num", "12.34E5"),
                T("id", "x")
            },
            {}
        });

        // ----------------------------------------------------
        // TEST 7: Whitespace skipping.
        // ----------------------------------------------------

        tests.push_back({
            "7. Tabs and newlines",
            "int\tvalue\n=\t12;",
            {
                T("KEYWORD", "int"),
                T("id", "value"),
                T("assign", "="),
                T("num", "12"),
                T("PUNCTUATION", ";")
            },
            {}
        });

        // ----------------------------------------------------
        // TEST 8: Invalid characters and error recovery.
        // ----------------------------------------------------

        tests.push_back({
            "8. Invalid characters and error recovery",
            "int x = 5 @;\n!",
            {
                T("KEYWORD", "int"),
                T("id", "x"),
                T("assign", "="),
                T("num", "5"),
                T("PUNCTUATION", ";")
            },
            {
                {'@', 1, 11},
                {'!', 2, 1}
            }
        });

        // ----------------------------------------------------
        // TEST 9: Empty input.
        // ----------------------------------------------------

        tests.push_back({
            "9. Empty input",
            "",
            {},
            {}
        });

        // ----------------------------------------------------
        // TEST 10: Multiple consecutive whitespace characters.
        // ----------------------------------------------------

        tests.push_back({
            "10. Whitespace-only input",
            " \t\n\r\n  ",
            {},
            {}
        });

        // ----------------------------------------------------
        // TEST 11: An invalid character between valid tokens.
        // ----------------------------------------------------

        tests.push_back({
            "11. Error between adjacent tokens",
            "int@x",
            {
                T("KEYWORD", "int"),
                T("id", "x")
            },
            {
                {'@', 1, 4}
            }
        });

        // ----------------------------------------------------
        // TEST 12: Identifier lengths and digit suffixes.
        // ----------------------------------------------------

        tests.push_back({
            "12. Identifiers with digit suffixes",
            "a Z abc ABC a1 Z9 abc123",
            {
                T("id", "a"),
                T("id", "Z"),
                T("id", "abc"),
                T("id", "ABC"),
                T("id", "a1"),
                T("id", "Z9"),
                T("id", "abc123")
            },
            {}
        });

        // ----------------------------------------------------
        // Run every test and print the final summary.
        // ----------------------------------------------------

        std::cout
            << "\n\n============================================================\n"
            << "4. TOKENIZATION TEST SUITES\n"
            << "============================================================\n";

        int passed = 0;

        for (const TestCase& test : tests) {
            if (runTest(lexer, test)) {
                ++passed;
            }
        }

        const int total = static_cast<int>(tests.size());
        const int failed = total - passed;

        std::cout
            << "\n\n============================================================\n"
            << "5. FINAL TEST SUMMARY\n"
            << "============================================================\n";

        std::cout << "Total test suites: " << total << '\n';
        std::cout << "Passed:            " << passed << '\n';
        std::cout << "Failed:            " << failed << '\n';

        if (failed == 0) {
            std::cout
                << "\nALL TEST SUITES PASSED.\n"
                << "Token sequences and lexical errors match "
                << "the expected results.\n";

            return 0;
        }

        std::cout
            << "\nSOME TESTS FAILED.\n"
            << "Inspect the mismatch details above.\n";

        return 1;
    } catch (const std::exception& exception) {
        std::cerr
            << "\nFATAL ERROR: "
            << exception.what()
            << '\n';

        return 2;
    }
}