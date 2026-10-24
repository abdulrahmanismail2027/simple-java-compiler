#include "automata/NFATransitionTable.h"

void NFATransitionTable::addTransition(const int from_state, const char input, const int to_state) {
    table[from_state][input].insert(to_state);
    states.insert(from_state);
    states.insert(to_state);

    if (input != EPSILON) {
        alphabet.insert(input);
    }
}

std::unordered_set<int> NFATransitionTable::getTransitions(const int current_state, const char input) const {
    if (const auto state_it = table.find(current_state); state_it != table.end()) {
        if (const auto input_it = state_it->second.find(input); input_it != state_it->second.end()) {
            return input_it->second;
        }
    }
    return {};
}

const std::map<int, std::map<char, std::unordered_set<int> > > &NFATransitionTable::getTable() const {
    return table;
}
