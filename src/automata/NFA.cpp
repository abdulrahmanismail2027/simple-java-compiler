#include "automata/NFA.h"
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

void NFA::addTransition(const int from_state, const char input, const int to_state) {
    transition_table.addTransition(from_state, input, to_state);
}

const NFATransitionTable &NFA::getTable() const {
    return transition_table;
}


NFA NFA::combine(const std::vector<NFA>& nfas) {
    if (nfas.empty()) {
        throw std::invalid_argument(
            "Cannot combine an empty list of NFAs"
        );
    }

    NFA combined(0);
    int nextState = 1;

    for (const NFA& source : nfas) {
        // Collect every state used by this source NFA.
        std::unordered_set<int> states;
        states.insert(source.getStartState());

        for (const auto& [from, transitions] :
             source.getTable().getTable()) {
            states.insert(from);

            for (const auto& [symbol, destinations] : transitions) {
                (void)symbol;
                states.insert(
                    destinations.begin(),
                    destinations.end()
                );
            }
        }

        for (const auto& [id, accept] :
             source.getAcceptStates()) {
            states.insert(id);
            states.insert(accept.id);
        }

        // Assign unique IDs to all states in this NFA.
        std::unordered_map<int, int> remap;

        for (const int oldState : states) {
            if (nextState == std::numeric_limits<int>::max()) {
                throw std::overflow_error(
                    "Too many NFA states"
                );
            }

            remap.emplace(oldState, nextState++);
        }

        // Connect the global start state to this NFA's start.
        combined.addTransition(
            0,
            EPSILON,
            remap.at(source.getStartState())
        );

        // Copy all transitions with remapped state IDs.
        for (const auto& [from, transitions] :
             source.getTable().getTable()) {
            for (const auto& [symbol, destinations] : transitions) {
                for (const int destination : destinations) {
                    combined.addTransition(
                        remap.at(from),
                        symbol,
                        remap.at(destination)
                    );
                }
            }
        }

        // Preserve accepting-state metadata.
        for (const auto& [id, accept] :
             source.getAcceptStates()) {
            (void)id;

            combined.addAcceptState(
                AcceptState(
                    remap.at(accept.id),
                    accept.priority,
                    accept.token_class
                )
            );
        }
    }

    return combined;
}