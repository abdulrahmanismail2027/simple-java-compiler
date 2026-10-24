#include "automata/DFA.h"

void DFA::addTransition(const int from_state, const char input, const int to_state) {
    transition_table.addTransition(from_state, input, to_state);
}

const DFATransitionTable &DFA::getTable() const {
    return transition_table;
}
