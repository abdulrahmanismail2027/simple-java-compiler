#include <gtest/gtest.h>

#include "automata/DFA.h"
#include "automata/NFA.h"

#include <string>
#include <unordered_set>

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

}

TEST(DFAConstruction, ConvertsEpsilonTransitionsAndPreservesAcceptPriority) {
    NFA nfa(0);
    nfa.addTransition(0, EPSILON, 1);
    nfa.addTransition(0, EPSILON, 2);
    nfa.addTransition(1, 'a', 1);
    nfa.addTransition(2, 'a', 3);
    nfa.addTransition(3, 'a', 3);
    nfa.addTransition(1, 'b', 4);
    nfa.addAcceptState(AcceptState(1, 2, "first"));
    nfa.addAcceptState(AcceptState(3, 1, "second"));

    const DFA dfa = DFA::fromNFA(nfa);

    EXPECT_EQ(dfa.getStartState(), 0);
    EXPECT_EQ(
        dfa.getTable().getAlphabet(),
        (std::unordered_set<char>{'a', 'b'}));
    EXPECT_EQ(dfa.getTable().getNextState(0, 'a'), 1);
    EXPECT_EQ(dfa.getTable().getNextState(0, 'b'), 2);

    ASSERT_TRUE(dfa.getAcceptStates().contains(0));
    EXPECT_EQ(dfa.getAcceptStates().at(0).token_class, "first");
    ASSERT_TRUE(dfa.getAcceptStates().contains(1));
    EXPECT_EQ(dfa.getAcceptStates().at(1).token_class, "second");
    EXPECT_EQ(dfa.getAcceptStates().at(1).priority, 1);
}

TEST(DFAConstruction, CreatesACompleteEmptySubsetSink) {
    NFA nfa(10);
    nfa.addTransition(10, 'a', 11);
    nfa.addTransition(99, 'b', 100);
    nfa.addAcceptState(AcceptState(11, 0, "letter"));

    const DFA dfa = DFA::fromNFA(nfa);

    const int accepting = dfa.getTable().getNextState(0, 'a');
    ASSERT_GE(accepting, 0);
    const int sink = dfa.getTable().getNextState(0, 'b');
    ASSERT_GE(sink, 0);
    EXPECT_EQ(dfa.getTable().getNextState(sink, 'a'), sink);
    EXPECT_EQ(dfa.getTable().getNextState(sink, 'b'), sink);
    EXPECT_EQ(dfa.getTable().getNextState(accepting, 'a'), sink);
    EXPECT_EQ(dfa.getTable().getNextState(accepting, 'b'), sink);
    EXPECT_FALSE(dfa.getAcceptStates().contains(sink));
    EXPECT_EQ(
        dfa.getTable().getAlphabet(),
        (std::unordered_set<char>{'a', 'b'}));
}

TEST(DFAConstruction, HandlesNFAWithoutTransitionsOrAcceptingStates) {
    const NFA nfa(42);

    const DFA dfa = DFA::fromNFA(nfa);

    EXPECT_EQ(dfa.getStartState(), 0);
    EXPECT_TRUE(dfa.getAcceptStates().empty());
    EXPECT_TRUE(dfa.getTable().getAlphabet().empty());
    EXPECT_EQ(dfa.getTable().getNextState(0, 'a'), -1);
    EXPECT_FALSE(accepts(dfa, ""));
    EXPECT_FALSE(accepts(dfa, "a"));
}

TEST(DFAConstruction, PreservesAnAcceptingStartStateAndEmptyStringAcceptance) {
    NFA nfa(17);
    nfa.addAcceptState(AcceptState(17, 4, "empty"));

    const DFA dfa = DFA::fromNFA(nfa);

    ASSERT_TRUE(dfa.getAcceptStates().contains(dfa.getStartState()));
    EXPECT_EQ(dfa.getAcceptStates().at(dfa.getStartState()).priority, 4);
    EXPECT_EQ(
        dfa.getAcceptStates().at(dfa.getStartState()).token_class,
        "empty");
    EXPECT_TRUE(accepts(dfa, ""));
    EXPECT_FALSE(accepts(dfa, "x"));
}

TEST(DFAConstruction, ComputesTransitiveEpsilonClosureWithCycles) {
    NFA nfa(5);
    nfa.addTransition(5, EPSILON, 6);
    nfa.addTransition(6, EPSILON, 7);
    nfa.addTransition(7, EPSILON, 6);
    nfa.addTransition(7, 'x', 8);
    nfa.addAcceptState(AcceptState(8, 0, "x"));

    const DFA dfa = DFA::fromNFA(nfa);

    EXPECT_TRUE(accepts(dfa, "x"));
    EXPECT_FALSE(accepts(dfa, ""));
    EXPECT_FALSE(accepts(dfa, "xx"));
}

TEST(DFAConstruction, AppliesEpsilonClosureAfterEachInputTransition) {
    NFA nfa(0);
    nfa.addTransition(0, 'a', 1);
    nfa.addTransition(1, EPSILON, 2);
    nfa.addTransition(2, 'b', 3);
    nfa.addAcceptState(AcceptState(3, 0, "ab"));

    const DFA dfa = DFA::fromNFA(nfa);

    EXPECT_TRUE(accepts(dfa, "ab"));
    EXPECT_FALSE(accepts(dfa, "a"));
    EXPECT_FALSE(accepts(dfa, "b"));
}

TEST(DFAConstruction, ConvertsSimpleLinearNFAAndRejectsOtherStrings) {
    NFA nfa(0);
    nfa.addTransition(0, 'a', 1);
    nfa.addTransition(1, 'b', 2);
    nfa.addAcceptState(AcceptState(2, 0, "ab"));

    const DFA dfa = DFA::fromNFA(nfa);

    EXPECT_TRUE(accepts(dfa, "ab"));
    EXPECT_FALSE(accepts(dfa, ""));
    EXPECT_FALSE(accepts(dfa, "a"));
    EXPECT_FALSE(accepts(dfa, "ba"));
    EXPECT_FALSE(accepts(dfa, "abb"));
}

TEST(DFAConstruction, ReusesDFAStateForEquivalentNFAStateSubsets) {
    NFA nfa(0);
    nfa.addTransition(0, 'a', 1);
    nfa.addTransition(0, 'b', 2);
    nfa.addTransition(1, 'c', 3);
    nfa.addTransition(2, 'c', 3);
    nfa.addAcceptState(AcceptState(3, 0, "c"));

    const DFA dfa = DFA::fromNFA(nfa);
    const int after_a = dfa.getTable().getNextState(dfa.getStartState(), 'a');
    const int after_b = dfa.getTable().getNextState(dfa.getStartState(), 'b');

    ASSERT_NE(after_a, after_b);
    const int after_ac = dfa.getTable().getNextState(after_a, 'c');
    const int after_bc = dfa.getTable().getNextState(after_b, 'c');
    EXPECT_EQ(after_ac, after_bc);
    EXPECT_TRUE(accepts(dfa, "ac"));
    EXPECT_TRUE(accepts(dfa, "bc"));
    EXPECT_FALSE(accepts(dfa, "a"));
    EXPECT_FALSE(accepts(dfa, "b"));
}

TEST(DFAConstruction, BreaksEqualPriorityTiesByNFAAcceptStateId) {
    NFA nfa(0);
    nfa.addTransition(0, EPSILON, 8);
    nfa.addTransition(0, EPSILON, 3);
    nfa.addAcceptState(AcceptState(8, 2, "higher-id"));
    nfa.addAcceptState(AcceptState(3, 2, "lower-id"));

    const DFA dfa = DFA::fromNFA(nfa);

    ASSERT_TRUE(dfa.getAcceptStates().contains(dfa.getStartState()));
    EXPECT_EQ(
        dfa.getAcceptStates().at(dfa.getStartState()).token_class,
        "lower-id");
    EXPECT_EQ(dfa.getAcceptStates().at(dfa.getStartState()).priority, 2);
}

TEST(DFAConstruction, KeepsUnreachableAcceptStatesOutOfDFA) {
    NFA nfa(0);
    nfa.addTransition(0, 'a', 1);
    nfa.addTransition(50, 'b', 51);
    nfa.addAcceptState(AcceptState(51, 0, "unreachable"));

    const DFA dfa = DFA::fromNFA(nfa);

    EXPECT_FALSE(accepts(dfa, "b"));
    EXPECT_TRUE(dfa.getAcceptStates().empty());
}

TEST(DFAConstruction, AcceptsAnEmptySubsetAsNonAcceptingSink) {
    NFA nfa(0);
    nfa.addTransition(0, 'a', 1);
    nfa.addTransition(1, 'b', 2);
    nfa.addAcceptState(AcceptState(2, 0, "ab"));

    const DFA dfa = DFA::fromNFA(nfa);
    const int sink = dfa.getTable().getNextState(dfa.getStartState(), 'b');

    ASSERT_GE(sink, 0);
    EXPECT_FALSE(dfa.getAcceptStates().contains(sink));
    EXPECT_EQ(dfa.getTable().getNextState(sink, 'a'), sink);
    EXPECT_EQ(dfa.getTable().getNextState(sink, 'b'), sink);
    EXPECT_FALSE(accepts(dfa, "b"));
    EXPECT_TRUE(accepts(dfa, "ab"));
}
