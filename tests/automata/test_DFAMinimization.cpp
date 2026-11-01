#include <gtest/gtest.h>

#include "automata/DFA.h"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

bool accepts(const DFA &dfa, const std::string &input) {
    int state = dfa.getStartState();
    for (const char symbol : input) {
        state = dfa.getTable().getNextState(state, symbol);
        if (state == -1) {
            return false;
        }
    }

    return dfa.getAcceptStates().contains(state);
}

std::size_t reachableStateCount(const DFA &dfa) {
    std::unordered_set<int> visited{dfa.getStartState()};
    std::vector<int> pending{dfa.getStartState()};
    const auto &alphabet = dfa.getTable().getAlphabet();

    while (!pending.empty()) {
        const int state = pending.back();
        pending.pop_back();
        for (const char symbol : alphabet) {
            const int next = dfa.getTable().getNextState(state, symbol);
            if (next != -1 && visited.insert(next).second) {
                pending.push_back(next);
            }
        }
    }

    return visited.size();
}

void expectEquivalentUpTo(
    const DFA &left,
    const DFA &right,
    const std::vector<char> &alphabet,
    const std::size_t max_length) {
    std::function<void(const std::string &, std::size_t)> check =
        [&](const std::string &word, const std::size_t remaining) {
            EXPECT_EQ(accepts(left, word), accepts(right, word))
                << "Acceptance differs for input: " << word;
            if (remaining == 0) {
                return;
            }
            for (const char symbol : alphabet) {
                check(word + symbol, remaining - 1);
            }
        };

    check("", max_length);
}

}

TEST(DFAMinimization, MergesEquivalentNonAcceptingStates) {
    DFA dfa(0);
    dfa.addTransition(0, 'a', 1);
    dfa.addTransition(0, 'b', 2);
    dfa.addTransition(1, 'a', 3);
    dfa.addTransition(1, 'b', 3);
    dfa.addTransition(2, 'a', 3);
    dfa.addTransition(2, 'b', 3);
    dfa.addTransition(3, 'a', 3);
    dfa.addTransition(3, 'b', 3);
    dfa.addAcceptState(AcceptState(3, 0, "token"));

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(reachableStateCount(minimized), 3);
    expectEquivalentUpTo(dfa, minimized, {'a', 'b'}, 5);
}

TEST(DFAMinimization, MergesAcceptingStatesWithIdenticalTokenMetadata) {
    DFA dfa(0);
    dfa.addTransition(0, 'a', 1);
    dfa.addTransition(0, 'b', 2);
    dfa.addTransition(1, 'a', 1);
    dfa.addTransition(1, 'b', 1);
    dfa.addTransition(2, 'a', 2);
    dfa.addTransition(2, 'b', 2);
    dfa.addAcceptState(AcceptState(1, 3, "identifier"));
    dfa.addAcceptState(AcceptState(2, 3, "identifier"));

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(reachableStateCount(minimized), 2);
    ASSERT_TRUE(minimized.getAcceptStates().contains(1));
    EXPECT_EQ(minimized.getAcceptStates().at(1).priority, 3);
    EXPECT_EQ(minimized.getAcceptStates().at(1).token_class, "identifier");
    expectEquivalentUpTo(dfa, minimized, {'a', 'b'}, 4);
}

TEST(DFAMinimization, DoesNotMergeAcceptingStatesWithDifferentTokenClasses) {
    DFA dfa(0);
    dfa.addTransition(0, 'a', 1);
    dfa.addTransition(0, 'b', 2);
    dfa.addTransition(1, 'a', 1);
    dfa.addTransition(1, 'b', 1);
    dfa.addTransition(2, 'a', 2);
    dfa.addTransition(2, 'b', 2);
    dfa.addAcceptState(AcceptState(1, 0, "keyword"));
    dfa.addAcceptState(AcceptState(2, 0, "identifier"));

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(reachableStateCount(minimized), 3);
    EXPECT_EQ(minimized.getAcceptStates().size(), 2);
}

TEST(DFAMinimization, DoesNotMergeAcceptingStatesWithDifferentPriorities) {
    DFA dfa(0);
    dfa.addTransition(0, 'a', 1);
    dfa.addTransition(0, 'b', 2);
    dfa.addTransition(1, 'a', 1);
    dfa.addTransition(1, 'b', 1);
    dfa.addTransition(2, 'a', 2);
    dfa.addTransition(2, 'b', 2);
    dfa.addAcceptState(AcceptState(1, 1, "token"));
    dfa.addAcceptState(AcceptState(2, 2, "token"));

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(reachableStateCount(minimized), 3);
    EXPECT_EQ(minimized.getAcceptStates().size(), 2);
}

TEST(DFAMinimization, RemovesUnreachableStates) {
    DFA dfa(0);
    dfa.addTransition(0, 'a', 1);
    dfa.addTransition(1, 'a', 1);
    dfa.addTransition(50, 'a', 51);
    dfa.addAcceptState(AcceptState(1, 0, "reachable"));
    dfa.addAcceptState(AcceptState(51, 0, "unreachable"));

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(reachableStateCount(minimized), 2);
    ASSERT_EQ(minimized.getAcceptStates().size(), 1);
    EXPECT_EQ(minimized.getAcceptStates().begin()->second.token_class, "reachable");
    expectEquivalentUpTo(dfa, minimized, {'a'}, 5);
}

TEST(DFAMinimization, CompletesPartialTransitionsWithRejectingSink) {
    DFA dfa(0);
    dfa.addTransition(0, 'a', 1);
    dfa.addTransition(0, 'b', 2);
    dfa.addTransition(1, 'a', 1);
    dfa.addTransition(2, 'b', 2);
    dfa.addAcceptState(AcceptState(1, 0, "a-path"));

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(minimized.getTable().getAlphabet(),
              (std::unordered_set<char>{'a', 'b'}));
    std::unordered_set<int> states = minimized.getTable().getStates();
    states.insert(minimized.getStartState());
    for (const int state : states) {
        EXPECT_NE(minimized.getTable().getNextState(state, 'a'), -1);
        EXPECT_NE(minimized.getTable().getNextState(state, 'b'), -1);
    }
    expectEquivalentUpTo(dfa, minimized, {'a', 'b'}, 5);
}

TEST(DFAMinimization, CollapsesAnEmptyLanguageToOneRejectingState) {
    DFA dfa(10);
    dfa.addTransition(10, 'a', 20);
    dfa.addTransition(20, 'a', 20);

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(reachableStateCount(minimized), 1);
    EXPECT_TRUE(minimized.getAcceptStates().empty());
    EXPECT_EQ(minimized.getTable().getNextState(0, 'a'), 0);
    EXPECT_FALSE(accepts(minimized, ""));
    EXPECT_FALSE(accepts(minimized, "aaaa"));
}

TEST(DFAMinimization, PreservesAcceptingStartStateWithNoAlphabet) {
    DFA dfa(27);
    dfa.addAcceptState(AcceptState(27, 4, "empty"));

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(minimized.getStartState(), 0);
    EXPECT_TRUE(minimized.getTable().getAlphabet().empty());
    ASSERT_TRUE(minimized.getAcceptStates().contains(0));
    EXPECT_EQ(minimized.getAcceptStates().at(0).token_class, "empty");
    EXPECT_TRUE(accepts(minimized, ""));
    EXPECT_FALSE(accepts(minimized, "x"));
}

TEST(DFAMinimization, UsesMultipleRefinementRoundsToDistinguishStates) {
    DFA dfa(0);
    dfa.addTransition(0, 'a', 1);
    dfa.addTransition(1, 'a', 2);
    dfa.addTransition(2, 'a', 3);
    dfa.addTransition(3, 'a', 4);
    dfa.addTransition(4, 'a', 4);
    dfa.addAcceptState(AcceptState(3, 0, "exactly-three"));

    const DFA minimized = dfa.minimize();

    EXPECT_EQ(reachableStateCount(minimized), 5);
    EXPECT_TRUE(accepts(minimized, "aaa"));
    EXPECT_FALSE(accepts(minimized, ""));
    EXPECT_FALSE(accepts(minimized, "aa"));
    EXPECT_FALSE(accepts(minimized, "aaaa"));
    expectEquivalentUpTo(dfa, minimized, {'a'}, 6);
}

TEST(DFAMinimization, PreservesAnAlreadyMinimalDFAAndIsIdempotent) {
    DFA dfa(9);
    dfa.addTransition(9, 'a', 10);
    dfa.addTransition(10, 'b', 11);
    dfa.addAcceptState(AcceptState(11, 0, "ab"));

    const DFA minimized = dfa.minimize();
    const DFA minimized_again = minimized.minimize();

    EXPECT_EQ(reachableStateCount(minimized), 4);
    EXPECT_EQ(reachableStateCount(minimized_again), 4);
    expectEquivalentUpTo(dfa, minimized, {'a', 'b'}, 5);
    expectEquivalentUpTo(minimized, minimized_again, {'a', 'b'}, 5);
}
