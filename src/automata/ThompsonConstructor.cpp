#include "automata/ThompsonConstructor.h"
#include "automata/AcceptState.h"
#include "automata/NFATransitionTable.h"
#include <stdexcept>
#include <utility>


NFA ThompsonConstructor::construct(
    const RegexNode* root,
    const std::string& tokenClass,
    int priority
) {
    if (root == nullptr) {
        throw std::invalid_argument(
            "Cannot construct an NFA from a null AST"
        );
    }

    ThompsonConstructor constructor;

    const NFAFragment fragment =
        constructor.buildNode(root);

    constructor.nfa.setStartState(fragment.start);

    // Preserve the token name and rule priority.
    constructor.nfa.addAcceptState(
        AcceptState(fragment.end, priority, tokenClass)
    );

    return std::move(constructor.nfa);
}

int ThompsonConstructor::createState() {
    return nextState++;
}

void ThompsonConstructor::addEpsilonTransition(int from, int to) {
    nfa.addTransition(from, EPSILON, to);
}

ThompsonConstructor::NFAFragment ThompsonConstructor::buildNode(const RegexNode* node) {
    if (node == nullptr) {
        throw std::invalid_argument("Cannot construct an NFA from a null AST node");
    }

    switch (node->type) {
        case RegexNodeType::Symbol:
            return buildSymbol(node->symbol);

        case RegexNodeType::Epsilon:
            return buildEpsilon();

        case RegexNodeType::Concatenation:
            return buildConcatenation(node);

        case RegexNodeType::Alternation:
            return buildAlternation(node);

        case RegexNodeType::KleeneStar:
            return buildKleeneStar(node);

        case RegexNodeType::PositiveClosure:
            return buildPositiveClosure(node);
    }

    throw std::invalid_argument("Unknown regular expression node type");
}

ThompsonConstructor::NFAFragment ThompsonConstructor::buildSymbol(char symbol) {
    const int start = createState();
    const int end = createState();

    nfa.addTransition(start, symbol, end);

    return {start, end};
}

ThompsonConstructor::NFAFragment ThompsonConstructor::buildEpsilon() {
    const int start = createState();
    const int end = createState();

    addEpsilonTransition(start, end);

    return {start, end};
}

ThompsonConstructor::NFAFragment ThompsonConstructor::buildConcatenation(const RegexNode* node) {
    requireBinaryChildren(node);

    const NFAFragment left = buildNode(node->left.get());
    const NFAFragment right = buildNode(node->right.get());

    addEpsilonTransition(left.end, right.start);

    return {left.start, right.end};
}

ThompsonConstructor::NFAFragment ThompsonConstructor::buildAlternation(const RegexNode* node) {
    requireBinaryChildren(node);

    const int start = createState();
    const int end = createState();

    const NFAFragment left = buildNode(node->left.get());
    const NFAFragment right = buildNode(node->right.get());

    // Split into either branch.
    addEpsilonTransition(start, left.start);
    addEpsilonTransition(start, right.start);

    // Join both branches at the common accept state.
    addEpsilonTransition(left.end, end);
    addEpsilonTransition(right.end, end);

    return {start, end};
}

ThompsonConstructor::NFAFragment ThompsonConstructor::buildKleeneStar(const RegexNode* node) {
    requireUnaryChild(node);

    const int start = createState();
    const int end = createState();

    const NFAFragment child = buildNode(node->left.get());

    // Enter A or skip it entirely (zero repetitions).
    addEpsilonTransition(start, child.start);
    addEpsilonTransition(start, end);

    // Repeat A or exit.
    addEpsilonTransition(child.end, child.start);
    addEpsilonTransition(child.end, end);

    return {start, end};
}

ThompsonConstructor::NFAFragment ThompsonConstructor::buildPositiveClosure(const RegexNode* node) {
    requireUnaryChild(node);

    const int start = createState();
    const int end = createState();

    const NFAFragment child = buildNode(node->left.get());

    // A must be entered at least once.
    addEpsilonTransition(start, child.start);

    // After the first traversal, repeat A or exit.
    addEpsilonTransition(child.end, child.start);
    addEpsilonTransition(child.end, end);

    return {start, end};
}

void ThompsonConstructor::requireBinaryChildren(const RegexNode* node) {
    if (node == nullptr || !node->left || !node->right)
        throw std::invalid_argument("Binary operator requires two children");
}

void ThompsonConstructor::requireUnaryChild(const RegexNode* node) {
    if (node == nullptr || !node->left || node->right)
        throw std::invalid_argument("Unary operator requires exactly one child");
}
