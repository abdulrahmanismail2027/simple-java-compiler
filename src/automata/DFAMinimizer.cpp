#include "automata/DFA.h"

#include <algorithm>
#include <limits>
#include <map>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

DFA DFA::minimize() const {
    std::vector<char> alphabet(
        transition_table.getAlphabet().begin(),
        transition_table.getAlphabet().end());
    std::sort(alphabet.begin(), alphabet.end());

    std::vector<int> states{start_state};
    std::map<int, std::size_t> state_indices{{start_state, 0}};
    std::queue<int> pending;
    pending.push(start_state);
    bool has_missing_transition = false;

    while (!pending.empty()) {
        const int state = pending.front();
        pending.pop();

        for (const char symbol : alphabet) {
            const int next_state = transition_table.getNextState(state, symbol);
            if (next_state == -1) {
                has_missing_transition = true;
                continue;
            }

            if (state_indices.emplace(next_state, states.size()).second) {
                states.push_back(next_state);
                pending.push(next_state);
            }
        }
    }

    const std::size_t real_state_count = states.size();
    const std::size_t sink_index = real_state_count;
    const std::size_t total_state_count =
        real_state_count + (has_missing_transition ? 1U : 0U);
    std::vector<std::vector<std::size_t>> transitions(
        total_state_count,
        std::vector<std::size_t>(alphabet.size()));

    for (std::size_t state_index = 0;
         state_index < real_state_count;
         ++state_index) {
        for (std::size_t symbol_index = 0;
             symbol_index < alphabet.size();
             ++symbol_index) {
            const int next_state = transition_table.getNextState(
                states[state_index], alphabet[symbol_index]);

            if (next_state == -1) {
                transitions[state_index][symbol_index] = sink_index;
            } else {
                transitions[state_index][symbol_index] =
                    state_indices.at(next_state);
            }
        }
    }

    if (has_missing_transition) {
        std::fill(
            transitions[sink_index].begin(),
            transitions[sink_index].end(),
            sink_index);
    }

    using AcceptSignature = std::tuple<bool, int, std::string>;
    std::map<AcceptSignature, int> initial_partition_ids;
    std::vector<AcceptSignature> accept_signatures;
    accept_signatures.reserve(total_state_count);

    for (const int state : states) {
        const auto accept = accept_states.find(state);
        const AcceptSignature signature =
            accept == accept_states.end()
                ? AcceptSignature{false, 0, ""}
                : AcceptSignature{
                      true, accept->second.priority, accept->second.token_class};
        accept_signatures.push_back(signature);
        initial_partition_ids.emplace(signature, 0);
    }
    if (has_missing_transition) {
        accept_signatures.emplace_back(false, 0, "");
        initial_partition_ids.emplace(AcceptSignature{false, 0, ""}, 0);
    }

    int next_partition_id = 0;
    for (auto &[signature, partition_id] : initial_partition_ids) {
        (void)signature;
        partition_id = next_partition_id++;
    }

    std::vector<int> partitions(total_state_count);
    for (std::size_t state_index = 0;
         state_index < total_state_count;
         ++state_index) {
        partitions[state_index] =
            initial_partition_ids.at(accept_signatures[state_index]);
    }

    while (true) {
        std::vector<std::vector<int>> signatures(total_state_count);
        std::map<std::vector<int>, int> refined_partition_ids;

        for (std::size_t state_index = 0;
             state_index < total_state_count;
             ++state_index) {
            std::vector<int> signature{partitions[state_index]};
            for (const std::size_t destination : transitions[state_index]) {
                signature.push_back(partitions[destination]);
            }
            refined_partition_ids.emplace(signature, 0);
            signatures[state_index] = std::move(signature);
        }

        next_partition_id = 0;
        for (auto &[signature, partition_id] : refined_partition_ids) {
            (void)signature;
            partition_id = next_partition_id++;
        }

        std::vector<int> refined_partitions(total_state_count);
        for (std::size_t state_index = 0;
             state_index < total_state_count;
             ++state_index) {
            refined_partitions[state_index] =
                refined_partition_ids.at(signatures[state_index]);
        }

        if (refined_partitions == partitions) {
            break;
        }
        partitions = std::move(refined_partitions);
    }

    std::vector<std::size_t> representatives(
        static_cast<std::size_t>(
            *std::max_element(partitions.begin(), partitions.end()) + 1));
    for (std::size_t state_index = 0;
         state_index < total_state_count;
         ++state_index) {
        representatives[static_cast<std::size_t>(partitions[state_index])] =
            state_index;
    }

    std::map<int, int> minimized_state_ids;
    std::vector<int> minimized_partitions;
    const int minimized_start_partition = partitions[0];
    minimized_state_ids.emplace(minimized_start_partition, 0);
    minimized_partitions.push_back(minimized_start_partition);
    DFA minimized(0);

    for (std::size_t current = 0;
         current < minimized_partitions.size();
         ++current) {
        const int partition = minimized_partitions[current];
        const std::size_t representative =
            representatives[static_cast<std::size_t>(partition)];
        const int minimized_state = static_cast<int>(current);

        if (representative < real_state_count) {
            const auto accept = accept_states.find(states[representative]);
            if (accept != accept_states.end()) {
                minimized.addAcceptState(AcceptState(
                    minimized_state,
                    accept->second.priority,
                    accept->second.token_class));
            }
        }

        for (std::size_t symbol_index = 0;
             symbol_index < alphabet.size();
             ++symbol_index) {
            const int destination_partition = partitions[
                transitions[representative][symbol_index]];
            auto destination =
                minimized_state_ids.find(destination_partition);
            if (destination == minimized_state_ids.end()) {
                if (minimized_state_ids.size() >
                    static_cast<std::size_t>(
                        std::numeric_limits<int>::max())) {
                    throw std::overflow_error("Too many minimized DFA states");
                }
                const int next_state =
                    static_cast<int>(minimized_state_ids.size());
                destination = minimized_state_ids.emplace(
                    destination_partition, next_state).first;
                minimized_partitions.push_back(destination_partition);
            }
            minimized.addTransition(
                minimized_state,
                alphabet[symbol_index],
                destination->second);
        }
    }

    return minimized;
}
