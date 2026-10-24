#include "automata/FA.h"

FA::FA(const int start) : start_state(start) {
}

void FA::setStartState(const int state) {
    start_state = state;
}

void FA::addAcceptState(const AcceptState &state) {
    accept_states.insert({state.id, state});
}

int FA::getStartState() const {
    return start_state;
}

const std::unordered_map<int, AcceptState> &FA::getAcceptStates() const {
    return accept_states;
}
