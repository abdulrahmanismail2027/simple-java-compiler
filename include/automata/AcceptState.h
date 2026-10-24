#pragma once
#include <string>
#include <limits>
#include <utility>

struct AcceptState {
    int id;
    int priority;
    std::string token_class;

    explicit AcceptState(const int id = -1,
                         const int priority = std::numeric_limits<int>::max(),
                         std::string token_class = "")
        : id(id), priority(priority), token_class(std::move(token_class)) {
    }
};
