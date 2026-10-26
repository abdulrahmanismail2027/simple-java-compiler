#pragma once

#include <string>
#include <unordered_set>

#include "io/RuleParser.h"

class RegexPreprocessor {
public:
    explicit RegexPreprocessor(const ParsedRules& rules);

    [[nodiscard]] std::string expand(const std::string& regex) const;

private:
    const ParsedRules& rules;

    std::string expandRegex(
        const std::string& regex,
        std::unordered_set<std::string>& expanding
    ) const;

    static std::string expandRanges(const std::string& regex);
};
