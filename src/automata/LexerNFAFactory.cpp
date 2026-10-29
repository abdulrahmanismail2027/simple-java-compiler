#include "automata/LexerNFAFactory.h"

#include "automata/ThompsonConstructor.h"
#include "regex/RegexParser.h"
#include "regex/RegexPreprocessor.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string decodePunctuation(const std::string& punctuation) {
    std::string result;
    result.reserve(punctuation.size());

    for (std::size_t i = 0; i < punctuation.size(); ++i) {
        if (punctuation[i] == '\\' && i + 1 < punctuation.size()) {
            const char next = punctuation[i + 1];

            // The rules file uses these escapes to represent
            // literal parentheses.
            if (next == '(' || next == ')') {
                result.push_back(next);
                ++i;
                continue;
            }
        }

        result.push_back(punctuation[i]);
    }

    return result;
}

} // namespace

NFA LexerNFAFactory::build(const ParsedRules& rules) {
    RegexPreprocessor preprocessor(rules);
    std::vector<NFA> tokenNFAs;

    int priority = 0;

    // 1. Keywords: always interpret as literal strings.
    for (const auto& keyword : rules.keywords) {
        auto ast = RegexParser::parseLiteral(keyword);

        tokenNFAs.push_back(
            ThompsonConstructor::construct(
                *ast, keyword, priority++
            )
        );
    }

    // 2. Punctuation: decode rule-file escapes, then interpret
    //    the result as a literal string.
    for (const auto& punctuation : rules.punctuations) {
        const std::string literal = decodePunctuation(punctuation);
        auto ast = RegexParser::parseLiteral(literal);

        tokenNFAs.push_back(
            ThompsonConstructor::construct(
                *ast, literal, priority++
            )
        );
    }

    // 3. Regular expressions: preserve regex escaping and syntax.
    for (const auto& [name, regex] : rules.regular_expressions) {
        const std::string expanded = preprocessor.expand(regex);

        RegexParser parser(expanded);
        auto ast = parser.parse();

        tokenNFAs.push_back(
            ThompsonConstructor::construct(
                *ast, name, priority++
            )
        );
    }

    if (tokenNFAs.empty()) {
        throw std::invalid_argument(
            "Cannot build a lexer NFA: no token rules provided"
        );
    }

    return NFA::combine(tokenNFAs);
}