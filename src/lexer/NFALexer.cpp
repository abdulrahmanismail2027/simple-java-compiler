#include "lexer/NFALexer.h"
#include "automata/NFATransitionTable.h"

#include <cctype>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

// ============================================================
// 1. EPSILON CLOSURE
//
// Find every state reachable using zero or more epsilon
// transitions, without consuming an input character.
// ============================================================

std::unordered_set<int> epsilonClosure(
    const NFA& nfa,
    std::unordered_set<int> states
) {
    std::vector<int> worklist(states.begin(), states.end());

    while (!worklist.empty()) {
        const int state = worklist.back();
        worklist.pop_back();

        const auto destinations =
            nfa.getTable().getTransitions(state, EPSILON);

        for (const int destination : destinations) {
            const auto [it, inserted] = states.insert(destination);
            (void)it;

            if (inserted) {
                worklist.push_back(destination);
            }
        }
    }

    return states;
}

// ============================================================
// 2. MOVE
//
// Find all states reachable after consuming exactly one
// character from any state in the current state set.
// ============================================================

std::unordered_set<int> move(
    const NFA& nfa,
    const std::unordered_set<int>& currentStates,
    char input
) {
    std::unordered_set<int> nextStates;

    for (const int state : currentStates) {
        const auto destinations =
            nfa.getTable().getTransitions(state, input);

        nextStates.insert(
            destinations.begin(),
            destinations.end()
        );
    }

    return nextStates;
}

// ============================================================
// 3. ACCEPTING STATE SELECTION
//
// A state is accepting if it appears in getAcceptStates().
//
// When multiple rules accept the same lexeme length, select
// the state with the smallest priority number.
// ============================================================

std::optional<AcceptState> bestAcceptingState(
    const NFA& nfa,
    const std::unordered_set<int>& currentStates
) {
    std::optional<AcceptState> best;
    const auto& acceptStates = nfa.getAcceptStates();

    for (const int state : currentStates) {
        const auto it = acceptStates.find(state);

        if (it == acceptStates.end()) {
            continue;
        }

        const AcceptState& candidate = it->second;

        if (!best.has_value() ||
            candidate.priority < best->priority) {
            best = candidate;
        }
    }

    return best;
}

// ============================================================
// 4. SOURCE POSITIONS
//
// Precompute line/column information for each input offset.
// CRLF is treated as one newline; standalone CR and LF are
// also handled as newline characters.
// ============================================================

struct SourcePositions {
    std::vector<std::size_t> lines;
    std::vector<std::size_t> columns;
};

SourcePositions calculatePositions(const std::string& source) {
    SourcePositions positions;

    positions.lines.resize(source.size() + 1);
    positions.columns.resize(source.size() + 1);

    std::size_t line = 1;
    std::size_t column = 1;

    for (std::size_t i = 0; i < source.size(); ++i) {
        positions.lines[i] = line;
        positions.columns[i] = column;

        if (source[i] == '\r') {
            ++line;
            column = 1;
        } else if (source[i] == '\n') {
            // If the previous character was CR, this is the
            // second character of CRLF, so don't count another line.
            if (i == 0 || source[i - 1] != '\r') {
                ++line;
            }

            column = 1;
        } else {
            ++column;
        }
    }

    positions.lines[source.size()] = line;
    positions.columns[source.size()] = column;

    return positions;
}

} // namespace

// ============================================================
// CONSTRUCTOR
// ============================================================

NFALexer::NFALexer(const NFA& nfa)
    : nfa_(nfa) {
}

// ============================================================
// TOKENIZATION
//
// At every token boundary:
//
//   1. Start from the NFA start state.
//   2. Compute its epsilon closure.
//   3. Consume input while possible transitions exist.
//   4. Remember each accepting prefix.
//   5. Choose the longest accepted prefix.
//   6. Break equal-length ties using token priority.
//   7. If nothing matches, report one invalid character and
//      continue scanning from the next character.
// ============================================================

TokenizationResult NFALexer::tokenize(
    const std::string& source,
    bool skipWhitespace
) const {
    TokenizationResult result;

    const SourcePositions positions =
        calculatePositions(source);

    std::size_t position = 0;

    while (position < source.size()) {
        // Whitespace is not represented as a token in the
        // current lexical rules, so skip it by default.
        if (skipWhitespace &&
            std::isspace(
                static_cast<unsigned char>(source[position])
            )) {
            ++position;
            continue;
        }

        std::unordered_set<int> initialStates{
            nfa_.getStartState()
        };

        auto currentStates =
            epsilonClosure(nfa_, std::move(initialStates));

        std::size_t cursor = position;
        std::size_t lastAcceptedEnd = position;

        std::optional<AcceptState> lastAcceptedToken;

        // Try to consume as much input as possible.
        while (cursor < source.size()) {
            auto nextStates = move(
                nfa_,
                currentStates,
                source[cursor]
            );

            if (nextStates.empty()) {
                break;
            }

            currentStates =
                epsilonClosure(nfa_, std::move(nextStates));

            ++cursor;

            const auto accepting =
                bestAcceptingState(nfa_, currentStates);

            if (accepting.has_value()) {
                // A later accepted prefix is longer, so it
                // replaces any previously accepted prefix.
                lastAcceptedToken = accepting;
                lastAcceptedEnd = cursor;
            }
        }

        if (lastAcceptedToken.has_value() &&
            lastAcceptedEnd > position) {
            Token token;

            token.tokenClass =
                lastAcceptedToken->token_class;

            token.lexeme = source.substr(
                position,
                lastAcceptedEnd - position
            );

            token.priority =
                lastAcceptedToken->priority;

            token.startOffset = position;
            token.endOffset = lastAcceptedEnd;

            token.line = positions.lines[position];
            token.column = positions.columns[position];

            result.tokens.push_back(std::move(token));

            // Resume from the end of the longest accepted token.
            position = lastAcceptedEnd;
        } else {
            // No token starts at this character.
            // Record the error and advance only one character,
            // preserving the opportunity to tokenize later input.
            LexicalError error;

            error.character = source[position];
            error.offset = position;
            error.line = positions.lines[position];
            error.column = positions.columns[position];

            result.errors.push_back(error);

            ++position;
        }
    }

    return result;
}