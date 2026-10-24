#pragma once
#include "TransitionTable.h"
#include <map>

class DFATransitionTable : public TransitionTable {
    std::map<int, std::map<char, int> > table;

public:
    void addTransition(int from_state, char input, int to_state) override;

    [[nodiscard]] int getNextState(int current_state, char input) const;

    [[nodiscard]] const std::map<int, std::map<char, int> > &getTable() const;
};
