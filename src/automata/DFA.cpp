#include "automata/DFA.h"
#include "automata/NFA.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

using StateSet = std::set<int>;

StateSet epsilonClosure(const NFA &nfa, StateSet states) {
    std::vector<int> pending(states.begin(), states.end());

    while (!pending.empty()) {
        const int state = pending.back();
        pending.pop_back();

        for (const int destination :
             nfa.getTable().getTransitions(state, EPSILON)) {
            if (states.insert(destination).second) {
                pending.push_back(destination);
            }
        }
    }

    return states;
}

}

DFA DFA::fromNFA(const NFA &nfa) {
    std::vector<StateSet> dfa_states;
    std::map<StateSet, int> state_ids;

    const auto addState = [&dfa_states, &state_ids](StateSet states) {
        if (const auto found = state_ids.find(states);
            found != state_ids.end()) {
            return found->second;
        }

        if (dfa_states.size() >
            static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::overflow_error("Too many DFA states");
        }

        const int id = static_cast<int>(dfa_states.size());
        state_ids.emplace(states, id);
        dfa_states.push_back(std::move(states));
        return id;
    };

    const int start_state = addState(
        epsilonClosure(nfa, StateSet{nfa.getStartState()}));
    DFA dfa(start_state);

    std::vector<char> alphabet(
        nfa.getTable().getAlphabet().begin(),
        nfa.getTable().getAlphabet().end());
    std::sort(alphabet.begin(), alphabet.end());

    for (std::size_t current_id = 0;
         current_id < dfa_states.size();
         ++current_id) {
        const StateSet current_states = dfa_states[current_id];
        const int dfa_state_id = static_cast<int>(current_id);

        const AcceptState *selected_accept = nullptr;
        for (const int nfa_state : current_states) {
            const auto accept = nfa.getAcceptStates().find(nfa_state);
            if (accept == nfa.getAcceptStates().end()) {
                continue;
            }

            if (selected_accept == nullptr ||
                accept->second.priority < selected_accept->priority ||
                (accept->second.priority == selected_accept->priority &&
                 accept->second.id < selected_accept->id)) {
                selected_accept = &accept->second;
            }
        }

        if (selected_accept != nullptr) {
            dfa.addAcceptState(AcceptState(
                dfa_state_id,
                selected_accept->priority,
                selected_accept->token_class));
        }

        for (const char symbol : alphabet) {
            StateSet destinations;
            for (const int nfa_state : current_states) {
                const auto next_states =
                    nfa.getTable().getTransitions(nfa_state, symbol);
                destinations.insert(next_states.begin(), next_states.end());
            }

            const int next_id = addState(
                epsilonClosure(nfa, std::move(destinations)));
            dfa.addTransition(dfa_state_id, symbol, next_id);
        }
    }

    return dfa;
}

void DFA::addTransition(const int from_state, const char input, const int to_state) {
    transition_table.addTransition(from_state, input, to_state);
}

const DFATransitionTable &DFA::getTable() const {
    return transition_table;
}
