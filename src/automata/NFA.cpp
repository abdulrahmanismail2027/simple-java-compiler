#include "automata/NFA.h"

void NFA::addTransition(const int from_state, const char input, const int to_state) {
    transition_table.addTransition(from_state, input, to_state);
}

const NFATransitionTable &NFA::getTable() const {
    return transition_table;
}
