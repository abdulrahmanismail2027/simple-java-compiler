
#include <gtest/gtest.h>

#include "automata/AcceptState.h"
#include "automata/NFA.h"
#include "automata/NFATransitionTable.h"
#include "automata/ThompsonConstructor.h"
#include "regex/RegexNode.h"
#include "regex/RegexParser.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

// ============================================================
// Helpers
// ============================================================

using StateSet = std::unordered_set<int>;

// Parse a regular expression and construct its NFA.
NFA buildNFA(const std::string& regex) {
    RegexParser parser(regex);
    const auto ast = parser.parse();

    return ThompsonConstructor::construct(ast.get());
}

// Compute the epsilon closure of a set of states.
//
// The epsilon closure contains every state reachable from
// the given states using zero or more epsilon transitions.
StateSet epsilonClosure(const NFA& nfa, StateSet states) {
    std::vector<int> pending(states.begin(), states.end());

    while (!pending.empty()) {
        const int current = pending.back();
        pending.pop_back();

        const StateSet destinations =
            nfa.getTable().getTransitions(current, EPSILON);

        for (const int destination : destinations) {
            if (states.insert(destination).second) {
                pending.push_back(destination);
            }
        }
    }

    return states;
}

// Simulate an NFA on an input string.
//
// At each input character:
// 1. Follow all transitions matching that character.
// 2. Compute epsilon closure of the resulting states.
//
// The string is accepted if any final state is accepting.
bool accepts(const NFA& nfa, const std::string& input) {
    StateSet currentStates =
        epsilonClosure(nfa, StateSet{nfa.getStartState()});

    for (const char symbol : input) {
        StateSet nextStates;

        for (const int state : currentStates) {
            const StateSet destinations =
                nfa.getTable().getTransitions(state, symbol);

            nextStates.insert(
                destinations.begin(),
                destinations.end()
            );
        }

        currentStates = epsilonClosure(nfa, nextStates);

        if (currentStates.empty()) {
            return false;
        }
    }

    for (const int state : currentStates) {
        if (nfa.getAcceptStates().contains(state)) {
            return true;
        }
    }

    return false;
}

// Verify that a transition exists.
void expectTransition(
    const NFA& nfa,
    int from,
    char symbol,
    int to
) {
    const StateSet destinations =
        nfa.getTable().getTransitions(from, symbol);

    EXPECT_TRUE(destinations.contains(to))
        << "Expected transition from state " << from
        << " to state " << to
        << " with symbol value "
        << static_cast<int>(symbol);
}

// Verify that an exact set of destinations exists.
void expectDestinations(
    const NFA& nfa,
    int from,
    char symbol,
    const StateSet& expected
) {
    EXPECT_EQ(
        nfa.getTable().getTransitions(from, symbol),
        expected
    ) << "Unexpected destinations from state " << from;
}

// Verify the NFA has exactly one accepting state.
void expectSingleAcceptState(const NFA& nfa, int expected) {
    const auto& acceptStates = nfa.getAcceptStates();

    ASSERT_EQ(acceptStates.size(), 1);
    EXPECT_TRUE(acceptStates.contains(expected));
}

} // namespace

// ============================================================
// 1. Symbol construction
// ============================================================

TEST(ThompsonConstructorTest, ConstructsSingleSymbol) {
    const NFA nfa = buildNFA("a");

    EXPECT_EQ(nfa.getStartState(), 0);
    expectSingleAcceptState(nfa, 1);

    expectDestinations(nfa, 0, 'a', StateSet{1});

    EXPECT_TRUE(
        nfa.getTable().getTransitions(0, EPSILON).empty()
    );

    EXPECT_TRUE(accepts(nfa, "a"));
    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "aa"));
}

// ============================================================
// 2. Epsilon construction
// ============================================================

TEST(ThompsonConstructorTest, ConstructsEpsilon) {
    const NFA nfa = buildNFA("\\L");

    EXPECT_EQ(nfa.getStartState(), 0);
    expectSingleAcceptState(nfa, 1);

    expectDestinations(nfa, 0, EPSILON, StateSet{1});

    EXPECT_TRUE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "a"));
    EXPECT_FALSE(accepts(nfa, "b"));
}

// ============================================================
// 3. Concatenation construction
// ============================================================

TEST(ThompsonConstructorTest, ConstructsConcatenation) {
    const NFA nfa = buildNFA("ab");

    EXPECT_EQ(nfa.getStartState(), 0);
    expectSingleAcceptState(nfa, 3);

    // Fragment for a.
    expectDestinations(nfa, 0, 'a', StateSet{1});

    // Connect the fragments.
    expectDestinations(nfa, 1, EPSILON, StateSet{2});

    // Fragment for b.
    expectDestinations(nfa, 2, 'b', StateSet{3});

    EXPECT_TRUE(accepts(nfa, "ab"));
    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "a"));
    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "ba"));
    EXPECT_FALSE(accepts(nfa, "abb"));
}

// ============================================================
// 4. Alternation construction
// ============================================================

TEST(ThompsonConstructorTest, ConstructsAlternation) {
    const NFA nfa = buildNFA("a|b");

    // State allocation:
    // 0 = new start
    // 1 = new accept
    // 2,3 = a fragment
    // 4,5 = b fragment

    EXPECT_EQ(nfa.getStartState(), 0);
    expectSingleAcceptState(nfa, 1);

    expectDestinations(nfa, 0, EPSILON, StateSet{2, 4});

    expectDestinations(nfa, 2, 'a', StateSet{3});
    expectDestinations(nfa, 4, 'b', StateSet{5});

    expectDestinations(nfa, 3, EPSILON, StateSet{1});
    expectDestinations(nfa, 5, EPSILON, StateSet{1});

    EXPECT_TRUE(accepts(nfa, "a"));
    EXPECT_TRUE(accepts(nfa, "b"));

    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "ab"));
    EXPECT_FALSE(accepts(nfa, "ba"));
    EXPECT_FALSE(accepts(nfa, "c"));
}

// ============================================================
// 5. Kleene star construction
// ============================================================

TEST(ThompsonConstructorTest, ConstructsKleeneStar) {
    const NFA nfa = buildNFA("a*");

    // State allocation:
    // 0 = new start
    // 1 = new accept
    // 2,3 = a fragment

    EXPECT_EQ(nfa.getStartState(), 0);
    expectSingleAcceptState(nfa, 1);

    expectDestinations(nfa, 0, EPSILON, StateSet{1, 2});
    expectDestinations(nfa, 2, 'a', StateSet{3});
    expectDestinations(nfa, 3, EPSILON, StateSet{1, 2});

    // Zero or more occurrences.
    EXPECT_TRUE(accepts(nfa, ""));
    EXPECT_TRUE(accepts(nfa, "a"));
    EXPECT_TRUE(accepts(nfa, "aa"));
    EXPECT_TRUE(accepts(nfa, "aaa"));
    EXPECT_TRUE(accepts(nfa, "aaaaaa"));

    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "ab"));
    EXPECT_FALSE(accepts(nfa, "ba"));
}

// ============================================================
// 6. Positive closure construction
// ============================================================

TEST(ThompsonConstructorTest, ConstructsPositiveClosure) {
    const NFA nfa = buildNFA("a+");

    EXPECT_EQ(nfa.getStartState(), 0);
    expectSingleAcceptState(nfa, 1);

    expectDestinations(nfa, 0, EPSILON, StateSet{2});
    expectDestinations(nfa, 2, 'a', StateSet{3});
    expectDestinations(nfa, 3, EPSILON, StateSet{1, 2});

    // At least one occurrence is required.
    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_TRUE(accepts(nfa, "a"));
    EXPECT_TRUE(accepts(nfa, "aa"));
    EXPECT_TRUE(accepts(nfa, "aaa"));

    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "ab"));
}

// ============================================================
// 7. Nested alternation and repetition
// ============================================================

TEST(ThompsonConstructorTest, ConstructsAlternationInsideStar) {
    const NFA nfa = buildNFA("(a|b)*");

    EXPECT_TRUE(accepts(nfa, ""));
    EXPECT_TRUE(accepts(nfa, "a"));
    EXPECT_TRUE(accepts(nfa, "b"));
    EXPECT_TRUE(accepts(nfa, "ab"));
    EXPECT_TRUE(accepts(nfa, "ba"));
    EXPECT_TRUE(accepts(nfa, "abba"));
    EXPECT_TRUE(accepts(nfa, "bbbaab"));

    EXPECT_FALSE(accepts(nfa, "c"));
    EXPECT_FALSE(accepts(nfa, "abc"));
    EXPECT_FALSE(accepts(nfa, "bacd"));
}

TEST(ThompsonConstructorTest, ConstructsAlternationInsidePlus) {
    const NFA nfa = buildNFA("(a|b)+");

    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_TRUE(accepts(nfa, "a"));
    EXPECT_TRUE(accepts(nfa, "b"));
    EXPECT_TRUE(accepts(nfa, "ab"));
    EXPECT_TRUE(accepts(nfa, "ba"));
    EXPECT_TRUE(accepts(nfa, "abba"));

    EXPECT_FALSE(accepts(nfa, "c"));
    EXPECT_FALSE(accepts(nfa, "abc"));
}

// ============================================================
// 8. Operator precedence
// ============================================================

TEST(ThompsonConstructorTest, RespectsConcatenationAndAlternation) {
    const NFA nfa = buildNFA("ab|c");

    EXPECT_TRUE(accepts(nfa, "ab"));
    EXPECT_TRUE(accepts(nfa, "c"));

    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "a"));
    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "ac"));
    EXPECT_FALSE(accepts(nfa, "abc"));
}

TEST(ThompsonConstructorTest, RespectsRepetitionPrecedence) {
    const NFA nfa = buildNFA("ab*");

    EXPECT_TRUE(accepts(nfa, "a"));
    EXPECT_TRUE(accepts(nfa, "ab"));
    EXPECT_TRUE(accepts(nfa, "abb"));
    EXPECT_TRUE(accepts(nfa, "abbb"));

    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "ba"));
    EXPECT_FALSE(accepts(nfa, "aba"));
}

// ============================================================
// 9. Nested concatenation
// ============================================================

TEST(ThompsonConstructorTest, ConstructsConcatenationWithGroup) {
    const NFA nfa = buildNFA("a(b|c)");

    EXPECT_TRUE(accepts(nfa, "ab"));
    EXPECT_TRUE(accepts(nfa, "ac"));

    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "a"));
    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "c"));
    EXPECT_FALSE(accepts(nfa, "abc"));
}

TEST(ThompsonConstructorTest, ConstructsConcatenationWithRepeatedGroup) {
    const NFA nfa = buildNFA("a(b|c)*");

    EXPECT_TRUE(accepts(nfa, "a"));
    EXPECT_TRUE(accepts(nfa, "ab"));
    EXPECT_TRUE(accepts(nfa, "ac"));
    EXPECT_TRUE(accepts(nfa, "abb"));
    EXPECT_TRUE(accepts(nfa, "abc"));
    EXPECT_TRUE(accepts(nfa, "acbc"));

    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "ba"));
    EXPECT_FALSE(accepts(nfa, "ad"));
}

// ============================================================
// 10. Epsilon within a concatenation
// ============================================================

TEST(ThompsonConstructorTest, HandlesEpsilonInsideConcatenation) {
    const NFA nfa = buildNFA("a\\Lb");

    // a followed by epsilon followed by b is equivalent
    // to the language containing only "ab".
    EXPECT_TRUE(accepts(nfa, "ab"));

    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "a"));
    EXPECT_FALSE(accepts(nfa, "b"));
    EXPECT_FALSE(accepts(nfa, "aab"));
}

// ============================================================
// 11. Comprehensive complex expression
// ============================================================

TEST(ThompsonConstructorTest, HandlesComplexExpression) {
    const NFA nfa = buildNFA("(a|b)*(c|d)+");

    // Any sequence of a/b, followed by at least one c/d.
    EXPECT_TRUE(accepts(nfa, "c"));
    EXPECT_TRUE(accepts(nfa, "d"));
    EXPECT_TRUE(accepts(nfa, "ac"));
    EXPECT_TRUE(accepts(nfa, "bd"));
    EXPECT_TRUE(accepts(nfa, "abc"));
    EXPECT_TRUE(accepts(nfa, "abbdc"));
    EXPECT_TRUE(accepts(nfa, "bbccdd"));

    EXPECT_FALSE(accepts(nfa, ""));
    EXPECT_FALSE(accepts(nfa, "a"));
    EXPECT_FALSE(accepts(nfa, "ab"));
    EXPECT_FALSE(accepts(nfa, "ca"));
    EXPECT_FALSE(accepts(nfa, "cba"));
    EXPECT_FALSE(accepts(nfa, "abcx"));
}

// ============================================================
// 12. Independent constructions
// ============================================================

TEST(ThompsonConstructorTest, EachConstructionStartsFromStateZero) {
    const NFA first = buildNFA("a");
    const NFA second = buildNFA("b");

    EXPECT_EQ(first.getStartState(), 0);
    EXPECT_EQ(second.getStartState(), 0);

    expectDestinations(first, 0, 'a', StateSet{1});
    expectDestinations(second, 0, 'b', StateSet{1});

    EXPECT_TRUE(accepts(first, "a"));
    EXPECT_FALSE(accepts(first, "b"));

    EXPECT_TRUE(accepts(second, "b"));
    EXPECT_FALSE(accepts(second, "a"));
}

// ============================================================
// 13. Invalid ASTs
// ============================================================

TEST(ThompsonConstructorTest, RejectsNullAst) {
    EXPECT_THROW(
        ThompsonConstructor::construct(
            static_cast<const RegexNode*>(nullptr)
        ),
        std::invalid_argument
    );
}

TEST(ThompsonConstructorTest, RejectsConcatenationWithoutChildren) {
    const RegexNode root(RegexNodeType::Concatenation);

    EXPECT_THROW(
        ThompsonConstructor::construct(root),
        std::invalid_argument
    );
}

TEST(ThompsonConstructorTest, RejectsAlternationWithoutChildren) {
    const RegexNode root(RegexNodeType::Alternation);

    EXPECT_THROW(
        ThompsonConstructor::construct(root),
        std::invalid_argument
    );
}

TEST(ThompsonConstructorTest, RejectsKleeneStarWithoutChild) {
    const RegexNode root(RegexNodeType::KleeneStar);

    EXPECT_THROW(
        ThompsonConstructor::construct(root),
        std::invalid_argument
    );
}

TEST(ThompsonConstructorTest, RejectsPositiveClosureWithoutChild) {
    const RegexNode root(RegexNodeType::PositiveClosure);

    EXPECT_THROW(
        ThompsonConstructor::construct(root),
        std::invalid_argument
    );
}

TEST(ThompsonConstructorTest, RejectsUnaryNodeWithTwoChildren) {
    RegexNode root(RegexNodeType::KleeneStar);

    root.left = std::make_unique<RegexNode>(
        RegexNodeType::Symbol
    );
    root.left->symbol = 'a';

    root.right = std::make_unique<RegexNode>(
        RegexNodeType::Symbol
    );
    root.right->symbol = 'b';

    EXPECT_THROW(
        ThompsonConstructor::construct(root),
        std::invalid_argument
    );
}
