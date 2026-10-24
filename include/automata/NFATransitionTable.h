#pragma once
#include "TransitionTable.h"
#include <map>
#include <unordered_set>

constexpr char EPSILON = '\0';

class NFATransitionTable : public TransitionTable {
    std::map<int, std::map<char, std::unordered_set<int> > > table;

public:
    void addTransition(int from_state, char input, int to_state) override;

    [[nodiscard]] std::unordered_set<int> getTransitions(int current_state, char input) const;

    [[nodiscard]] const std::map<int, std::map<char, std::unordered_set<int> > > &getTable() const;
};
