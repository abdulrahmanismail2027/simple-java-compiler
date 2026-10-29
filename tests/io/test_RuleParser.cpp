#include <gtest/gtest.h>
#include "io/RuleParser.h"
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>

static std::string createTempFile(const std::string& filename, const std::string& content) {
    auto path = std::filesystem::temp_directory_path() / filename;
    std::ofstream out(path);
    out << content;
    return path.string();
}

TEST(RuleParserTest, ParseKeywords) {
    std::string file = createTempFile("test_keywords.txt", "{ boolean int float }\n");
    RuleParser parser(file);
    const ParsedRules rules = parser.parse();

    ASSERT_EQ(rules.keywords.size(), 3);
    EXPECT_EQ(rules.keywords[0], "boolean");
    EXPECT_EQ(rules.keywords[1], "int");
    EXPECT_EQ(rules.keywords[2], "float");

    std::filesystem::remove(file);
}

TEST(RuleParserTest, ParsePunctuations) {
    std::string file = createTempFile("test_punc.txt", "[ ; , \\( \\) { } ]\n");
    RuleParser parser(file);
    const ParsedRules rules = parser.parse();

    ASSERT_GE(rules.punctuations.size(), 6);
    EXPECT_EQ(rules.punctuations[0], ";");
    EXPECT_EQ(rules.punctuations[2], "\\(");
    EXPECT_EQ(rules.punctuations[3], "\\)");

    std::filesystem::remove(file);
}

TEST(RuleParserTest, ParseRegularDefinitions) {
    std::string file = createTempFile("test_defs.txt", "digit = 0-9\nletter = a-z\n");
    RuleParser parser(file);
    const ParsedRules rules = parser.parse();

    EXPECT_EQ(rules.regular_definitions.at("digit"), "0-9");
    EXPECT_EQ(rules.regular_definitions.at("letter"), "a-z");

    std::filesystem::remove(file);
}

TEST(RuleParserTest, ParseRegularExpressions) {
    std::string file = createTempFile("test_expr.txt", "assign: =\nrelop: <=\n");
    RuleParser parser(file);
    const ParsedRules rules = parser.parse();

    ASSERT_EQ(rules.regular_expressions.size(), 2);
    EXPECT_EQ(rules.regular_expressions[0].first, "assign");
    EXPECT_EQ(rules.regular_expressions[0].second, "=");
    EXPECT_EQ(rules.regular_expressions[1].first, "relop");
    EXPECT_EQ(rules.regular_expressions[1].second, "<=");

    std::filesystem::remove(file);
}

TEST(RuleParserTest, ThrowsOnMissingFile) {
    RuleParser parser("non_existent_file_12345.txt");
    EXPECT_THROW((void)parser.parse(), std::runtime_error);
}

TEST(RuleParserTest, ThrowsOnMissingOperator) {
    std::string file = createTempFile("test_missing_op.txt", "digit 0-9\n");
    RuleParser parser(file);

    EXPECT_THROW((void)parser.parse(), std::runtime_error);

    std::filesystem::remove(file);
}

TEST(RuleParserTest, ThrowsOnMissingLHS) {
    std::string file = createTempFile("test_missing_lhs.txt", "= 0-9\n");
    RuleParser parser(file);

    EXPECT_THROW((void)parser.parse(), std::runtime_error);

    std::filesystem::remove(file);
}

TEST(RuleParserTest, ThrowsOnMissingRHS) {
    std::string file = createTempFile("test_missing_rhs.txt", "empty_def =\n");
    RuleParser parser(file);

    EXPECT_THROW((void)parser.parse(), std::runtime_error);

    std::filesystem::remove(file);
}

TEST(RuleParserTest, ThrowsOnMalformedPunctuation) {
    std::string file = createTempFile("test_bad_punc.txt", "[ ; , \\( \n");
    RuleParser parser(file);

    EXPECT_THROW((void)parser.parse(), std::runtime_error);

    std::filesystem::remove(file);
}

TEST(RuleParserTest, ThrowsOnMissingClosingBracket) {
    std::string file = createTempFile("test_missing_bracket.txt", "{ boolean int float \n");
    RuleParser parser(file);

    EXPECT_THROW((void)parser.parse(), std::runtime_error);

    std::filesystem::remove(file);
}
