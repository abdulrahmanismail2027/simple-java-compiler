#pragma once

#include "automata/NFA.h"

#include <cstddef>
#include <string>
#include <vector>

struct Token {
    std::string tokenClass;
    std::string lexeme;
    int priority = 0;

    // [startOffset, endOffset)
    std::size_t startOffset = 0;
    std::size_t endOffset = 0;

    // One-based source positions.
    std::size_t line = 1;
    std::size_t column = 1;
};

struct LexicalError {
    char character = '\0';
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct TokenizationResult {
    std::vector<Token> tokens;
    std::vector<LexicalError> errors;

    [[nodiscard]] bool success() const noexcept {
        return errors.empty();
    }
};

class NFALexer {
public:
    explicit NFALexer(const NFA& nfa);

    [[nodiscard]] TokenizationResult tokenize(
        const std::string& source,
        bool skipWhitespace = true
    ) const;

private:
    // The NFA must outlive this lexer.
    const NFA& nfa_;
};