#include "regex/RegexPreprocessor.h"

#include <stdexcept>
#include <algorithm>

RegexPreprocessor::RegexPreprocessor(const ParsedRules& rules) : rules(rules) {}

std::string RegexPreprocessor::expand(const std::string& regex) const {
    std::unordered_set<std::string> expanding;
    return expandRegex(regex, expanding);
}

std::string RegexPreprocessor::expandRanges(const std::string& regex) {
    std::string result;

    for (std::size_t i = 0; i < regex.size();) {
        // Preserve escaped characters, including escaped '-'.
        if (regex[i] == '\\') {
            if (i + 1 >= regex.size()) {
                throw std::runtime_error("Incomplete escape in regular expression");
            }

            result += regex[i++];
            result += regex[i++];
            continue;
        }

        // Expand simple ranges such as a-z and 0-9.
        if (i + 2 < regex.size() &&
            regex[i + 1] == '-' &&
            regex[i] != '(' &&
            regex[i] != ')' &&
            regex[i] != '|' &&
            regex[i + 2] != '(' &&
            regex[i + 2] != ')' &&
            regex[i + 2] != '|') {

            const unsigned char first =
                static_cast<unsigned char>(regex[i]);
            const unsigned char last =
                static_cast<unsigned char>(regex[i + 2]);

            if (last < first) {
                throw std::runtime_error("Invalid descending character range");
            }

            result += '(';

            for (unsigned int c = first; c <= last; ++c) {
                if (c != first) {
                    result += '|';
                }
                result += static_cast<char>(c);
            }

            result += ')';
            i += 3;
            continue;
        }

        result += regex[i++];
    }

    return result;
}

std::string RegexPreprocessor::expandRegex(
    const std::string& regex,
    std::unordered_set<std::string>& expanding
) const {
    const std::string ranged = expandRanges(regex);
    std::string result;

    for (std::size_t i = 0; i < ranged.size();) {
        // Never interpret escaped characters as definition names.
        if (ranged[i] == '\\') {
            if (i + 1 >= ranged.size()) {
                throw std::runtime_error("Incomplete escape in regular expression");
            }

            result += ranged[i++];
            result += ranged[i++];
            continue;
        }

        // Find the longest definition name at this position.
        auto best = rules.regular_definitions.end();

        for (auto it = rules.regular_definitions.begin();
             it != rules.regular_definitions.end();
             ++it) {

            const std::string& name = it->first;

            if (name.empty() || name.size() <=
                    (best == rules.regular_definitions.end()
                         ? 0
                         : best->first.size())) {
                continue;
            }

            if (ranged.compare(i, name.size(), name) == 0) {
                best = it;
            }
        }

        if (best == rules.regular_definitions.end()) {
            result += ranged[i++];
            continue;
        }

        const std::string& name = best->first;

        if (expanding.contains(name)) {
            throw std::runtime_error(
                "Circular regular definition detected: " + name
            );
        }

        expanding.insert(name);

        // Parentheses preserve the definition's precedence.
        result += '(';
        result += expandRegex(best->second, expanding);
        result += ')';

        expanding.erase(name);
        i += name.size();
    }

    return result;
}
