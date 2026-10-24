#pragma once
#include <string>


struct Token {
    std::string token_class;
    std::string attribute_value;

    explicit Token(const std::string &t_class = "", const std::string &attr_val = "")
        : token_class(t_class), attribute_value(attr_val) {
    }
};
