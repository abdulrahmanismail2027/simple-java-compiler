#pragma once
#include "automata/FA.h"
#include "automata/DFATransitionTable.h"

class NFA;

class DFA : public FA {
    DFATransitionTable transition_table;

public:
    using FA::FA;

    [[nodiscard]] static DFA fromNFA(const NFA &nfa);
    [[nodiscard]] DFA minimize() const;

    void addTransition(int from_state, char input, int to_state) override;

    [[nodiscard]] const DFATransitionTable &getTable() const;
};
