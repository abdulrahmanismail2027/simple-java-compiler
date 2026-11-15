#include "automata/DFA.h"
#include "automata/LexerNFAFactory.h"
#include "io/RuleParser.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

std::set<int> getStateIds(const DFA& dfa) {
    const auto& table_states = dfa.getTable().getStates();
    std::set<int> states(table_states.begin(), table_states.end());
    states.insert(dfa.getStartState());

    for (const auto& [state, accept] : dfa.getAcceptStates()) {
        (void)accept;
        states.insert(state);
    }

    return states;
}

std::string displaySymbol(const char symbol) {
    switch (symbol) {
        case '\0':
            return "\\0";
        case '\n':
            return "\\n";
        case '\r':
            return "\\r";
        case '\t':
            return "\\t";
        case '\\':
            return "\\\\";
        case '\'':
            return "\\'";
        default:
            return std::string(1, symbol);
    }
}

void printTransitionTable(const DFA& dfa) {
    const auto states = getStateIds(dfa);
    const auto& transition_table = dfa.getTable();
    const auto& transitions = transition_table.getTable();
    const auto& accepting_states = dfa.getAcceptStates();

    std::vector<char> alphabet(
        transition_table.getAlphabet().begin(),
        transition_table.getAlphabet().end());
    std::sort(alphabet.begin(), alphabet.end());

    std::cout << "\nMinimized DFA transition table\n";
    std::cout << "Start state: " << dfa.getStartState() << '\n';
    std::cout << "Input symbols:";
    for (const char symbol : alphabet) {
        std::cout << " '" << displaySymbol(symbol) << '\'';
    }
    if (alphabet.empty()) {
        std::cout << " none";
    }
    std::cout << "\n\n";
    std::cout << "State | Start | Accepting (token class; priority) | "
                 "Transitions (input -> destination; absent = no transition)\n";

    for (const int state : states) {
        std::cout << state << " | "
                  << (state == dfa.getStartState() ? "yes" : "no") << " | ";

        const auto accept = accepting_states.find(state);
        if (accept == accepting_states.end()) {
            std::cout << "no";
        } else {
            std::cout << "yes: " << accept->second.token_class
                      << " (priority " << accept->second.priority << ')';
        }

        std::cout << " | ";
        const auto state_transitions = transitions.find(state);
        if (state_transitions == transitions.end() ||
            state_transitions->second.empty()) {
            std::cout << '-';
        } else {
            bool first = true;
            for (const auto& [symbol, destination] :
                 state_transitions->second) {
                if (!first) {
                    std::cout << ", ";
                }
                std::cout << '\'' << displaySymbol(symbol)
                          << "' -> " << destination;
                first = false;
            }
        }
        std::cout << '\n';
    }
}

void generateStandaloneLexer(const DFA& dfa, const std::string& path) {
    std::ofstream source(path);
    if (!source) {
        throw std::runtime_error(
            "Could not open generated lexer source file at " + path);
    }

    const auto states = getStateIds(dfa);
    const auto& transitions = dfa.getTable().getTable();

    std::vector<std::pair<int, AcceptState>> accepting_states;
    accepting_states.reserve(dfa.getAcceptStates().size());
    for (const auto& [state, accept] : dfa.getAcceptStates()) {
        accepting_states.emplace_back(state, accept);
    }
    std::sort(
        accepting_states.begin(),
        accepting_states.end(),
        [](const auto& left, const auto& right) {
            return left.first < right.first;
        });

    source << R"cpp(#include <cctype>
#include <deque>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

struct Token {
    std::string token_class;
    std::string attribute_value;

    explicit Token(
        const std::string& token_class = "",
        const std::string& attribute_value = "")
        : token_class(token_class), attribute_value(attribute_value) {}
};

class SymbolTable {
    std::unordered_set<std::string> table;

public:
    void insert(const std::string& identifier) {
        table.insert(identifier);
    }

    bool lookup(const std::string& identifier) const {
        return table.find(identifier) != table.end();
    }
};

struct AcceptInfo {
    std::string token_class;
    int priority;
};

)cpp"
           << "const int kStartState = "
           << dfa.getStartState() << ";\n"
              "const std::vector<int> kStates = {";

    bool first = true;
    for (const int state : states) {
        if (!first) {
            source << ", ";
        }
        source << state;
        first = false;
    }
    source << "};\n\n"
              "const std::map<int, std::map<char, int>> kTransitions = {\n";

    for (const auto& [state, outgoing] : transitions) {
        source << "    {" << state << ", {";
        bool first_transition = true;
        for (const auto& [symbol, destination] : outgoing) {
            if (!first_transition) {
                source << ", ";
            }
            source << '{' << std::quoted(std::string(1, symbol))
                   << "[0], " << destination << '}';
            first_transition = false;
        }
        source << "}},\n";
    }

    source << "};\n\n"
              "const std::map<int, AcceptInfo> kAcceptingStates = {\n";
    for (const auto& [state, accept] : accepting_states) {
        source << "    {" << state << ", {"
               << std::quoted(accept.token_class) << ", "
               << accept.priority << "}},\n";
    }
    source << R"cpp(};

int nextState(const int state, const char input) {
    const auto row = kTransitions.find(state);
    if (row == kTransitions.end()) {
        return -1;
    }
    const auto transition = row->second.find(input);
    return transition == row->second.end() ? -1 : transition->second;
}

class Lexer {
public:
    explicit Lexer(std::istream& input) : input_(input) {}

    std::optional<Token> nextToken() {
        while (ensureAvailable(0)) {
            if (std::isspace(
                    static_cast<unsigned char>(lookahead_.front()))) {
                consumeFront();
                continue;
            }

            int state = kStartState;
            std::size_t last_accepting_length = 0;
            AcceptInfo last_accepting_info{
                "", std::numeric_limits<int>::max()};
            std::size_t cursor = 0;

            while (ensureAvailable(cursor)) {
                const int next = nextState(state, lookahead_[cursor]);
                if (next == -1) {
                    break;
                }

                state = next;
                ++cursor;
                const auto accepting = kAcceptingStates.find(state);
                if (accepting != kAcceptingStates.end() &&
                    (cursor > last_accepting_length ||
                     (cursor == last_accepting_length &&
                      accepting->second.priority <
                          last_accepting_info.priority))) {
                    last_accepting_length = cursor;
                    last_accepting_info = accepting->second;
                }
            }

            if (last_accepting_length == 0) {
                std::cerr << "Lexical error at position " << position_
                          << ": unrecognized character '"
                          << lookahead_.front() << "'\n";
                consumeFront();
                continue;
            }

            std::string lexeme;
            lexeme.reserve(last_accepting_length);
            for (std::size_t i = 0; i < last_accepting_length; ++i) {
                lexeme.push_back(lookahead_[i]);
            }
            for (std::size_t i = 0; i < last_accepting_length; ++i) {
                consumeFront();
            }

            if (last_accepting_info.token_class == "id") {
                symbol_table_.insert(lexeme);
            }
            return Token(last_accepting_info.token_class, lexeme);
        }

        return std::nullopt;
    }

private:
    bool ensureAvailable(const std::size_t offset) {
        while (lookahead_.size() <= offset && !end_of_input_) {
            char next = '\0';
            if (input_.get(next)) {
                lookahead_.push_back(next);
            } else if (input_.bad() || !input_.eof()) {
                throw std::runtime_error("Failed to read source input");
            } else {
                end_of_input_ = true;
            }
        }
        return lookahead_.size() > offset;
    }

    void consumeFront() {
        lookahead_.pop_front();
        ++position_;
    }

    std::istream& input_;
    std::deque<char> lookahead_;
    SymbolTable symbol_table_;
    std::size_t position_ = 0;
    bool end_of_input_ = false;
};

#ifndef GENERATED_LEXER_NO_MAIN
int main(int argc, char* argv[]) {
    std::ifstream input_file;
    std::istream* input = &std::cin;
    if (argc > 1) {
        input_file.open(argv[1], std::ios::binary);
        if (!input_file) {
            std::cerr << "Could not open source input file: "
                      << argv[1] << '\n';
            return 1;
        }
        input = &input_file;
    }

    try {
        Lexer lexer(*input);
        while (true) {
            const std::optional<Token> token = lexer.nextToken();
            if (!token) {
                break;
            }
            std::cout << "Token class: " << token->token_class
                      << ", attribute: " << token->attribute_value << '\n';
        }
    } catch (const std::runtime_error& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
#endif
)cpp";

    source.close();
    if (!source) {
        throw std::runtime_error(
            "Failed while writing generated lexer source file at " + path);
    }
}

}

int main(const int argc, char* argv[]) {
    const std::string rules_path =
        argc > 1 ? argv[1] : "examples/lexical_rules.txt";
    const std::string generated_path =
        argc > 2 ? argv[2] : "generated_lexer.cpp";

    try {
        const ParsedRules rules = RuleParser(rules_path).parse();
        const NFA nfa = LexerNFAFactory::build(rules);
        const DFA dfa = DFA::fromNFA(nfa);
        const DFA minimized_dfa = dfa.minimize();

        std::cout << "DFA states before minimization: "
                  << getStateIds(dfa).size() << '\n';
        std::cout << "DFA states after minimization: "
                  << getStateIds(minimized_dfa).size() << '\n';
        printTransitionTable(minimized_dfa);
        generateStandaloneLexer(minimized_dfa, generated_path);
        std::cout << "\nGenerated standalone lexical analyzer: "
                  << generated_path << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
