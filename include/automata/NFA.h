#pragma once
#include "automata/FA.h"
#include "automata/NFATransitionTable.h"

class NFA : public FA {
    NFATransitionTable transition_table;

public:
    using FA::FA;

    void addTransition(int from_state, char input, int to_state) override;

    [[nodiscard]] const NFATransitionTable &getTable() const;
};
