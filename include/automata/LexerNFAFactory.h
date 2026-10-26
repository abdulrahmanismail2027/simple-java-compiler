#pragma once

#include "automata/NFA.h"
#include "io/RuleParser.h"

class LexerNFAFactory {
public:
    // Builds a unified NFA from the parsed lexical rules.
    static NFA build(const ParsedRules& rules);
};
