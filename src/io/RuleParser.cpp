#include "io/RuleParser.h"
#include <fstream>
#include <stdexcept>

RuleParser::RuleParser(std::string path) : file_path(std::move(path)) {}

void RuleParser::skipWhitespace() {
    while (pos < current_line.length() && std::isspace(current_line[pos])) {
        pos++;
    }
}

char RuleParser::consume() {
    if (pos >= current_line.length()) return '\0';
    return current_line[pos++];
}

std::string RuleParser::consumeWord(const std::string& delimiters) {
    skipWhitespace();
    std::string word;

    while (pos < current_line.length()) {
        const char c = current_line[pos];

        if (std::isspace(c) || delimiters.find(c) != std::string::npos) {
            break;
        }

        if (c == '\\') {
            word += consume();
            word += consume();
        } else {
            word += consume();
        }
    }
    return word;
}

void RuleParser::parseKeywords(ParsedRules& rules) {
    consume();
    while (true) {
        std::string word = consumeWord("}");
        if (word.empty()) break;
        rules.keywords.push_back(word);
    }

    skipWhitespace();
    if (consume() != '}') {
        throw std::runtime_error("Syntax Error: Missing closing '}' for keywords list.");
    }
}

void RuleParser::parsePunctuations(ParsedRules& rules) {
    consume();
    while (true) {
        std::string punc = consumeWord("]");
        if (punc.empty()) break;
        rules.punctuations.push_back(punc);
    }

    skipWhitespace();
    if (consume() != ']') {
        throw std::runtime_error("Syntax Error: Missing closing ']' for punctuations list.");
    }
}

void RuleParser::parseRule(ParsedRules& rules) {
    std::string lhs = consumeWord("=:");
    if (lhs.empty()) {
        throw std::runtime_error("Syntax Error: Missing left-hand side in rule definition.");
    }

    skipWhitespace();
    const char op = consume();
    if (op != '=' && op != ':') {
        throw std::runtime_error("Syntax Error: Expected '=' or ':' after left-hand side '" + lhs + "'.");
    }

    std::string rhs;
    while (true) {
        std::string chunk = consumeWord();
        if (chunk.empty()) break;
        rhs += chunk;
    }

    if (rhs.empty()) {
        throw std::runtime_error("Syntax Error: Missing right-hand side for rule '" + lhs + "'.");
    }

    if (op == '=') {
        rules.regular_definitions[lhs] = rhs;
    } else if (op == ':') {
        rules.regular_expressions.emplace_back(lhs, rhs);
    }
}

ParsedRules RuleParser::parse() {
    ParsedRules rules;
    std::ifstream file(file_path);

    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open lexical rules file at " + file_path);
    }

    while (std::getline(file, current_line)) {
        pos = 0;
        skipWhitespace();

        if (pos >= current_line.length()) continue;

        if (current_line[pos] == '{') {
            parseKeywords(rules);
        } else if (current_line[pos] == '[') {
            parsePunctuations(rules);
        } else {
            parseRule(rules);
        }
    }

    return rules;
}