#pragma once
#include <string>
#include <unordered_set>

class SymbolTable {
    std::unordered_set<std::string> table;

public:
    void insert(const std::string &id);

    bool lookup(const std::string &id) const;
};
