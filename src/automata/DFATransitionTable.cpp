#include "automata/DFATransitionTable.h"

void DFATransitionTable::addTransition(const int from_state, const char input, const int to_state) {
    table[from_state][input] = to_state;
    states.insert(from_state);
    states.insert(to_state);
    alphabet.insert(input);
}

int DFATransitionTable::getNextState(const int current_state, const char input) const {
    if (const auto state_it = table.find(current_state); state_it != table.end()) {
        if (const auto input_it = state_it->second.find(input); input_it != state_it->second.end()) {
            return input_it->second;
        }
    }
    return -1;
}

const std::map<int, std::map<char, int> > &DFATransitionTable::getTable() const {
    return table;
}
