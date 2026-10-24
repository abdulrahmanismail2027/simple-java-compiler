#pragma once
#include <unordered_set>

class TransitionTable {
protected:
    std::unordered_set<int> states;
    std::unordered_set<char> alphabet;

public:
    virtual ~TransitionTable() = default;

    virtual void addTransition(int from_state, char input, int to_state) = 0;

    [[nodiscard]] const std::unordered_set<int> &getStates() const { return states; }
    [[nodiscard]] const std::unordered_set<char> &getAlphabet() const { return alphabet; }
};
