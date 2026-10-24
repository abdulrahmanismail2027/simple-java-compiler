#include <gtest/gtest.h>
#include "io/RuleParser.h"
#include <fstream>
#include <string>
#include <stdexcept>

static void createTempFile(const std::string& filename, const std::string& content) {
        std::ofstream out(filename);
        out << content;
    }

TEST(RuleParserTest, ParseKeywords) {
    createTempFile("test_keywords.txt", "{ boolean int float }\n");
    RuleParser parser("test_keywords.txt");
    const ParsedRules rules = parser.parse();

    ASSERT_EQ(rules.keywords.size(), 3);
    EXPECT_EQ(rules.keywords[0], "boolean");
    EXPECT_EQ(rules.keywords[1], "int");
    EXPECT_EQ(rules.keywords[2], "float");
}

TEST(RuleParserTest, ParsePunctuations) {
    createTempFile("test_punc.txt", "[ ; , \\( \\) { } ]\n");
    RuleParser parser("test_punc.txt");
    const ParsedRules rules = parser.parse();

    ASSERT_GE(rules.punctuations.size(), 6);
    EXPECT_EQ(rules.punctuations[0], ";");
    EXPECT_EQ(rules.punctuations[2], "\\(");
    EXPECT_EQ(rules.punctuations[3], "\\)");
}

TEST(RuleParserTest, ParseRegularDefinitions) {
    createTempFile("test_defs.txt", "digit = 0-9\nletter = a-z\n");
    RuleParser parser("test_defs.txt");
    const ParsedRules rules = parser.parse();

    EXPECT_EQ(rules.regular_definitions.at("digit"), "0-9");
    EXPECT_EQ(rules.regular_definitions.at("letter"), "a-z");
}

TEST(RuleParserTest, ParseRegularExpressions) {
    createTempFile("test_expr.txt", "assign: =\nrelop: <=\n");
    RuleParser parser("test_expr.txt");
    const ParsedRules rules = parser.parse();

    ASSERT_EQ(rules.regular_expressions.size(), 2);
    EXPECT_EQ(rules.regular_expressions[0].first, "assign");
    EXPECT_EQ(rules.regular_expressions[0].second, "=");
    EXPECT_EQ(rules.regular_expressions[1].first, "relop");
    EXPECT_EQ(rules.regular_expressions[1].second, "<=");
}

TEST(RuleParserTest, ThrowsOnMissingFile) {
    RuleParser parser("non_existent_file_12345.txt");
    EXPECT_THROW((void)parser.parse(), std::runtime_error);
}

TEST(RuleParserTest, ThrowsOnMissingOperator) {
    createTempFile("test_missing_op.txt", "digit 0-9\n");
    RuleParser parser("test_missing_op.txt");

    EXPECT_THROW((void)parser.parse(), std::runtime_error);
}

TEST(RuleParserTest, ThrowsOnMissingLHS) {
    // Starts immediately with '=' or ':'
    createTempFile("test_missing_lhs.txt", "= 0-9\n");
    RuleParser parser("test_missing_lhs.txt");

    EXPECT_THROW((void)parser.parse(), std::runtime_error);
}

TEST(RuleParserTest, ThrowsOnMissingRHS) {
    // Has LHS and operator, but nothing after it
    createTempFile("test_missing_rhs.txt", "empty_def =\n");
    RuleParser parser("test_missing_rhs.txt");

    EXPECT_THROW((void)parser.parse(), std::runtime_error);
}

TEST(RuleParserTest, ThrowsOnMalformedPunctuation) {
    // Missing closing bracket for punctuation
    createTempFile("test_bad_punc.txt", "[ ; , \\( \n");
    RuleParser parser("test_bad_punc.txt");

    EXPECT_THROW((void)parser.parse(), std::runtime_error);
}

TEST(RuleParserTest, ThrowsOnMissingClosingBracket) {
    // Missing the closing '}' for keywords
    createTempFile("test_missing_bracket.txt", "{ boolean int float \n");
    RuleParser parser("test_missing_bracket.txt");

    EXPECT_THROW((void)parser.parse(), std::runtime_error);
}