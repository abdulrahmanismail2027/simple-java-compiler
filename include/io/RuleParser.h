#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

struct ParsedRules {
    std::unordered_map<std::string, std::string> regular_definitions;
    std::vector<std::pair<std::string, std::string> > regular_expressions;
    std::vector<std::string> keywords;
    std::vector<std::string> punctuations;
};

class RuleParser {
    std::string file_path;
    std::string current_line;
    size_t pos = 0;

    void skipWhitespace();

    char consume();

    std::string consumeWord(const std::string &delimiters = "");

    void parseKeywords(ParsedRules &rules);

    void parsePunctuations(ParsedRules &rules);

    void parseRule(ParsedRules &rules);

public:
    explicit RuleParser(std::string path);

    [[nodiscard]] ParsedRules parse();
};
