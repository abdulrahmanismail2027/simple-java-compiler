
#pragma once

#include "automata/NFA.h"
#include "regex/RegexNode.h"

class ThompsonConstructor {
private:
    struct NFAFragment {
        int start;
        int end;
    };

    NFA nfa;
    int nextState = 0;

    // State and transition helpers
    int createState();
    void addEpsilonTransition(int from, int to);

    // Recursive construction
    NFAFragment buildNode(const RegexNode* node);

    // Individual Thompson construction rules
    NFAFragment buildSymbol(char symbol);
    NFAFragment buildEpsilon();
    NFAFragment buildConcatenation(const RegexNode* node);
    NFAFragment buildAlternation(const RegexNode* node);
    NFAFragment buildKleeneStar(const RegexNode* node);
    NFAFragment buildPositiveClosure(const RegexNode* node);

    // AST validation
    static void requireBinaryChildren(const RegexNode* node);
    static void requireUnaryChild(const RegexNode* node);

public:
    [[nodiscard]] static NFA construct(
        const RegexNode* root,
        const std::string& tokenClass = "",
        int priority = std::numeric_limits<int>::max()
    );

    [[nodiscard]] static NFA construct(
        const RegexNode& root,
        const std::string& tokenClass = "",
        int priority = std::numeric_limits<int>::max()
    ) {
        return construct(&root, tokenClass, priority);
    }
};
