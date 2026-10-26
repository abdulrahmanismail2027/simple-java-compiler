#include "regex/RegexParser.h"

#include <stdexcept>
#include <utility>
#include <memory>
#include <string>
#include "regex/RegexNode.h"

RegexParser::RegexParser(std::string regex) : regex(std::move(regex)) {}

std::unique_ptr<RegexNode> RegexParser::parse() {
    pos = 0;

    if (regex.empty()) {
        throw std::runtime_error("Regular expression cannot be empty");
    }

    auto root = parseExpression();

    if (pos != regex.size()) {
        throw std::runtime_error("Unexpected character at position " + std::to_string(pos));
    }

    return root;
}

// expression := concatenation ('|' concatenation)*
std::unique_ptr<RegexNode> RegexParser::parseExpression() {
    auto node = parseConcatenation();

    while (pos < regex.size() && regex[pos] == '|') {
        ++pos;

        if (pos >= regex.size() || regex[pos] == '|' || regex[pos] == ')') {
            throw std::runtime_error("Missing expression after '|'");
        }

        auto right = parseConcatenation();

        auto parent = std::make_unique<RegexNode>(RegexNodeType::Alternation);

        parent->left = std::move(node);
        parent->right = std::move(right);

        node = std::move(parent);
    }

    return node;
}

// concatenation := repetition+
std::unique_ptr<RegexNode> RegexParser::parseConcatenation() {
    if (!startsAtom()) {
        throw std::runtime_error("Expected an expression");
    }

    auto node = parseRepetition();

    while (startsAtom()) {
        auto right = parseRepetition();

        auto parent = std::make_unique<RegexNode>(RegexNodeType::Concatenation);

        parent->left = std::move(node);
        parent->right = std::move(right);

        node = std::move(parent);
    }

    return node;
}

// repetition := atom ('*' | '+')*
std::unique_ptr<RegexNode> RegexParser::parseRepetition() {
    auto node = parseAtom();

    bool hasPostfixOperator = false;

    while (pos < regex.size() &&
           (regex[pos] == '*' || regex[pos] == '+')) {

        if (hasPostfixOperator) {
            throw std::runtime_error(
                "Repeated postfix operators are not allowed"
            );
        }

        char op = regex[pos++];

        RegexNodeType type =
            (op == '*')
                ? RegexNodeType::KleeneStar
                : RegexNodeType::PositiveClosure;

        auto parent = std::make_unique<RegexNode>(type);
        parent->left = std::move(node);

        node = std::move(parent);
        hasPostfixOperator = true;
    }

    return node;
}

// atom := symbol | '\L' | '\escaped-symbol' | '(' expression ')'
std::unique_ptr<RegexNode> RegexParser::parseAtom() {
    if (pos >= regex.size()) {
        throw std::runtime_error("Unexpected end of regular expression");
    }

    char c = regex[pos++];

    if (c == '(') {
        if (pos < regex.size() && regex[pos] == ')') {
            throw std::runtime_error("Empty parentheses are not allowed; use \\L for epsilon");
        }

        auto node = parseExpression();

        if (pos >= regex.size() || regex[pos] != ')') {
            throw std::runtime_error("Missing closing parenthesis");
        }

        ++pos;
        return node;
    }

    if (c == '\\') {
        if (pos >= regex.size()) {
            throw std::runtime_error("Incomplete escape sequence");
        }

        char escaped = regex[pos++];

        if (escaped == 'L') {
            return std::make_unique<RegexNode>(RegexNodeType::Epsilon);
        }

        auto node = std::make_unique<RegexNode>(RegexNodeType::Symbol);

        node->symbol = escaped;
        return node;
    }

    if (c == ')' || c == '|' || c == '*' || c == '+') {
        throw std::runtime_error("Unexpected operator in regular expression");
    }

    auto node = std::make_unique<RegexNode>(RegexNodeType::Symbol);

    node->symbol = c;
    return node;
}

bool RegexParser::startsAtom() const {
    if (pos >= regex.size()) {
        return false;
    }

    char c = regex[pos];

    return c != ')' && c != '|';
}

std::unique_ptr<RegexNode> RegexParser::parseLiteral(
    const std::string& literal
) {
    if (literal.empty()) {
        throw std::invalid_argument(
            "A literal token cannot be empty"
        );
    }

    auto root = std::make_unique<RegexNode>(
        RegexNodeType::Symbol
    );
    root->symbol = literal[0];

    for (std::size_t i = 1; i < literal.size(); ++i) {
        auto concatenation = std::make_unique<RegexNode>(
            RegexNodeType::Concatenation
        );

        concatenation->left = std::move(root);

        auto symbol = std::make_unique<RegexNode>(
            RegexNodeType::Symbol
        );
        symbol->symbol = literal[i];

        concatenation->right = std::move(symbol);
        root = std::move(concatenation);
    }

    return root;
}
