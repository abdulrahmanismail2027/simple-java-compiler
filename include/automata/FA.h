#pragma once
#include "automata/AcceptState.h"
#include <unordered_map>

class FA {
protected:
    int start_state;
    std::unordered_map<int, AcceptState> accept_states;

public:
    explicit FA(int start = 0);

    virtual ~FA() = default;

    void setStartState(int state);

    void addAcceptState(const AcceptState &state);

    virtual void addTransition(int from_state, char input, int to_state) = 0;

    [[nodiscard]] int getStartState() const;

    [[nodiscard]] const std::unordered_map<int, AcceptState> &getAcceptStates() const;
};
