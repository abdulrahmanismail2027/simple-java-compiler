#pragma once

#include "regex/RegexNode.h"

#include <memory>
#include <string>
#include <unordered_map>

class RegexParser {
private:
    std::string regex;
    size_t pos = 0;

    std::unique_ptr<RegexNode> parseExpression();
    std::unique_ptr<RegexNode> parseConcatenation();
    std::unique_ptr<RegexNode> parseRepetition();
    std::unique_ptr<RegexNode> parseAtom();

    bool startsAtom() const;

public:
    explicit RegexParser(std::string regex);

    std::unique_ptr<RegexNode> parse();

    static std::unique_ptr<RegexNode> parseLiteral(
        const std::string& literal
    );
};
