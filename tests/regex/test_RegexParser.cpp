
#include <gtest/gtest.h>

#include "../../include/regex/RegexParser.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace {

using NodePtr = std::unique_ptr<RegexNode>;

NodePtr symbol(char c) {
    auto node = std::make_unique<RegexNode>(
        RegexNodeType::Symbol
    );
    node->symbol = c;
    return node;
}

NodePtr epsilon() {
    return std::make_unique<RegexNode>(
        RegexNodeType::Epsilon
    );
}

NodePtr unary(RegexNodeType type, NodePtr child) {
    auto node = std::make_unique<RegexNode>(type);
    node->left = std::move(child);
    return node;
}

NodePtr binary(
    RegexNodeType type,
    NodePtr left,
    NodePtr right
) {
    auto node = std::make_unique<RegexNode>(type);
    node->left = std::move(left);
    node->right = std::move(right);
    return node;
}

void expectTreesEqual(
    const RegexNode* actual,
    const RegexNode* expected,
    const std::string& path = "root"
) {
    ASSERT_NE(actual, nullptr) << path << ": actual node is null";
    ASSERT_NE(expected, nullptr) << path << ": expected node is null";

    EXPECT_EQ(actual->type, expected->type)
        << path << ": node types differ";

    if (actual->type == RegexNodeType::Symbol &&
        expected->type == RegexNodeType::Symbol) {
        EXPECT_EQ(actual->symbol, expected->symbol)
            << path << ": symbols differ";
    }

    const bool actualHasLeft = actual->left != nullptr;
    const bool expectedHasLeft = expected->left != nullptr;

    EXPECT_EQ(actualHasLeft, expectedHasLeft)
        << path << ": left-child presence differs";

    const bool actualHasRight = actual->right != nullptr;
    const bool expectedHasRight = expected->right != nullptr;

    EXPECT_EQ(actualHasRight, expectedHasRight)
        << path << ": right-child presence differs";

    if (actualHasLeft && expectedHasLeft) {
        expectTreesEqual(
            actual->left.get(),
            expected->left.get(),
            path + ".left"
        );
    }

    if (actualHasRight && expectedHasRight) {
        expectTreesEqual(
            actual->right.get(),
            expected->right.get(),
            path + ".right"
        );
    }
}

} // namespace

TEST(RegexParserTest, ParsesSingleSymbol) {
    RegexParser parser("a");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, RegexNodeType::Symbol);
    EXPECT_EQ(ast->symbol, 'a');
}

TEST(RegexParserTest, ParsesConcatenation) {
    RegexParser parser("ab");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->type, RegexNodeType::Concatenation);

    ASSERT_NE(ast->left, nullptr);
    EXPECT_EQ(ast->left->type, RegexNodeType::Symbol);
    EXPECT_EQ(ast->left->symbol, 'a');

    ASSERT_NE(ast->right, nullptr);
    EXPECT_EQ(ast->right->type, RegexNodeType::Symbol);
    EXPECT_EQ(ast->right->symbol, 'b');
}

TEST(RegexParserTest, ParsesAlternation) {
    RegexParser parser("a|b");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->type, RegexNodeType::Alternation);

    ASSERT_NE(ast->left, nullptr);
    EXPECT_EQ(ast->left->symbol, 'a');

    ASSERT_NE(ast->right, nullptr);
    EXPECT_EQ(ast->right->symbol, 'b');
}

TEST(RegexParserTest, ParsesKleeneStar) {
    RegexParser parser("a*");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->type, RegexNodeType::KleeneStar);

    ASSERT_NE(ast->left, nullptr);
    EXPECT_EQ(ast->left->type, RegexNodeType::Symbol);
    EXPECT_EQ(ast->left->symbol, 'a');
}

TEST(RegexParserTest, ParsesPositiveClosure) {
    RegexParser parser("a+");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->type, RegexNodeType::PositiveClosure);

    ASSERT_NE(ast->left, nullptr);
    EXPECT_EQ(ast->left->type, RegexNodeType::Symbol);
    EXPECT_EQ(ast->left->symbol, 'a');
}

TEST(RegexParserTest, ParsesParenthesizedExpression) {
    RegexParser parser("(a|b)");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, RegexNodeType::Alternation);
    EXPECT_EQ(ast->left->symbol, 'a');
    EXPECT_EQ(ast->right->symbol, 'b');
}

TEST(RegexParserTest, ParsesNestedExpression) {
    RegexParser parser("a(b|c)*");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->type, RegexNodeType::Concatenation);

    ASSERT_NE(ast->left, nullptr);
    EXPECT_EQ(ast->left->type, RegexNodeType::Symbol);
    EXPECT_EQ(ast->left->symbol, 'a');

    ASSERT_NE(ast->right, nullptr);
    ASSERT_EQ(ast->right->type, RegexNodeType::KleeneStar);

    ASSERT_NE(ast->right->left, nullptr);
    EXPECT_EQ(
        ast->right->left->type,
        RegexNodeType::Alternation
    );

    EXPECT_EQ(ast->right->left->left->symbol, 'b');
    EXPECT_EQ(ast->right->left->right->symbol, 'c');
}

TEST(RegexParserTest, RespectsOperatorPrecedence) {
    RegexParser parser("a|bc*");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->type, RegexNodeType::Alternation);

    ASSERT_NE(ast->left, nullptr);
    EXPECT_EQ(ast->left->type, RegexNodeType::Symbol);
    EXPECT_EQ(ast->left->symbol, 'a');

    ASSERT_NE(ast->right, nullptr);
    ASSERT_EQ(ast->right->type, RegexNodeType::Concatenation);

    EXPECT_EQ(ast->right->left->symbol, 'b');
    ASSERT_EQ(
        ast->right->right->type,
        RegexNodeType::KleeneStar
    );
    EXPECT_EQ(ast->right->right->left->symbol, 'c');
}

TEST(RegexParserTest, ParsesEpsilon) {
    RegexParser parser("\\L");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, RegexNodeType::Epsilon);
}

TEST(RegexParserTest, ParsesEscapedSymbol) {
    RegexParser parser("\\(");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, RegexNodeType::Symbol);
    EXPECT_EQ(ast->symbol, '(');
}

TEST(RegexParserTest, ParsesMultipleOperators) {
    RegexParser parser("(a|b)(c|d)+");
    const auto ast = parser.parse();

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->type, RegexNodeType::Concatenation);

    ASSERT_NE(ast->left, nullptr);
    EXPECT_EQ(ast->left->type, RegexNodeType::Alternation);

    ASSERT_NE(ast->right, nullptr);
    EXPECT_EQ(
        ast->right->type,
        RegexNodeType::PositiveClosure
    );

    ASSERT_NE(ast->right->left, nullptr);
    EXPECT_EQ(
        ast->right->left->type,
        RegexNodeType::Alternation
    );
}

TEST(RegexParserTest, ThrowsOnEmptyExpression) {
    RegexParser parser("");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ThrowsOnMissingRightOperand) {
    RegexParser parser("a|");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ThrowsOnMissingLeftOperand) {
    RegexParser parser("|a");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ThrowsOnUnexpectedLeadingOperator) {
    RegexParser parser("*a");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ThrowsOnMissingClosingParenthesis) {
    RegexParser parser("(a|b");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ThrowsOnUnexpectedClosingParenthesis) {
    RegexParser parser("a|b)");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ThrowsOnEmptyParentheses) {
    RegexParser parser("()");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ThrowsOnRepeatedPostfixOperators) {
    RegexParser parser("a**");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ThrowsOnIncompleteEscapeSequence) {
    RegexParser parser("\\");

    EXPECT_THROW(
        (void)parser.parse(),
        std::runtime_error
    );
}

TEST(RegexParserTest, ParsesComplexRegexAndMatchesEntireExpectedTree) {
    // Regex: (a|b)*(c|d)+\L
    RegexParser parser("(a|b)*(c|d)+\\L");

    const auto actual = parser.parse();

    // Expected tree:
    //
    // Concatenation
    // ├── Concatenation
    // │   ├── KleeneStar
    // │   │   └── Alternation
    // │   │       ├── Symbol('a')
    // │   │       └── Symbol('b')
    // │   └── PositiveClosure
    // │       └── Alternation
    // │           ├── Symbol('c')
    // │           └── Symbol('d')
    // └── Epsilon

    auto expected = binary(
        RegexNodeType::Concatenation,

        binary(
            RegexNodeType::Concatenation,

            unary(
                RegexNodeType::KleeneStar,
                binary(
                    RegexNodeType::Alternation,
                    symbol('a'),
                    symbol('b')
                )
            ),

            unary(
                RegexNodeType::PositiveClosure,
                binary(
                    RegexNodeType::Alternation,
                    symbol('c'),
                    symbol('d')
                )
            )
        ),

        epsilon()
    );

    ASSERT_NE(actual, nullptr);
    ASSERT_NE(expected, nullptr);

    expectTreesEqual(actual.get(), expected.get());
}
