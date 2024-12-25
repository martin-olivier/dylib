/**
 * @file format.cpp
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2024 Martin Olivier
 *
 * This library is released under MIT license
 */

#include <string>

static void replace_occurrences(std::string &input, const std::string &keyword, const std::string &replacement) {
    size_t pos = 0;

    if (keyword.empty())
        return;

    while ((pos = input.find(keyword, pos)) != std::string::npos) {
        input.replace(pos, keyword.length(), replacement);
        pos += replacement.length();
    }
}

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__MINGW32__)

static void add_space_after_comma(std::string &input) {
    std::string result;

    for (char c : input) {
        if (c == ',') {
            result += ", ";
        } else {
            result += c;
        }
    }

    input = result;
}

std::string format_symbol(std::string input) {
    replace_occurrences(input, "(class ", "(");
    replace_occurrences(input, "<class ", "<");
    replace_occurrences(input, ",class ", ",");

    replace_occurrences(input, "(struct ", "(");
    replace_occurrences(input, "<struct ", "<");
    replace_occurrences(input, ",struct ", ",");

    replace_occurrences(input, "> >", ">>");
    replace_occurrences(input, ">const", "> const");

    add_space_after_comma(input);

    return input;
}

#else

static void add_sym_separator(std::string &input, char symbol) {
    size_t pos = 0;

    if (input.empty()) {
        return;
    }

    while ((pos = input.find(symbol, pos)) != std::string::npos) {
        if (pos && input[pos - 1] != ' ' && input[pos - 1] != '&' && input[pos - 1] != '*') {
            input.replace(pos, 1, std::string(" ") + symbol);
            pos += 2;
        } else {
            pos += 1;
        }
    }
}

std::string format_symbol(std::string input) {
    replace_occurrences(input, "std::__1::", "std::");
    replace_occurrences(input, "std::__cxx11::", "std::");

    replace_occurrences(input, "[abi:cxx11]", "");
    replace_occurrences(input, "[abi:ue170006]", "");

    replace_occurrences(input, "()", "(void)");
    replace_occurrences(input, "> >", ">>");

    add_sym_separator(input, '*');
    add_sym_separator(input, '&');

    return input;
}

#endif

