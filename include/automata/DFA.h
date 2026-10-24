#pragma once
#include "automata/FA.h"
#include "automata/DFATransitionTable.h"

class DFA : public FA {
    DFATransitionTable transition_table;

public:
    using FA::FA;

    void addTransition(int from_state, char input, int to_state) override;

    [[nodiscard]] const DFATransitionTable &getTable() const;
};
