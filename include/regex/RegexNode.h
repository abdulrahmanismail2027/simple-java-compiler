#pragma once

#include <memory>
#include <string>

enum class RegexNodeType {
    Symbol,
    Epsilon,
    Concatenation,
    Alternation,
    KleeneStar,
    PositiveClosure
};

struct RegexNode {
    RegexNodeType type;

    // Used for Symbol nodes.
    char symbol = '\0';

    // Used by unary and binary operators.
    std::unique_ptr<RegexNode> left;
    std::unique_ptr<RegexNode> right;

    explicit RegexNode(RegexNodeType type) : type(type) {}
};
