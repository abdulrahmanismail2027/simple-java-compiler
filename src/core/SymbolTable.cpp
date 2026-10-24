#include "core/SymbolTable.h"

void SymbolTable::insert(const std::string &id) {
    table.insert(id);
}

bool SymbolTable::lookup(const std::string &id) const {
    return table.contains(id);
}
