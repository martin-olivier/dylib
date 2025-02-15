/**
 * @file format.cpp
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2025 Martin Olivier
 *
 * This library is released under MIT license
 */

#include <string>

static void replace_occurrences(std::string &symbol, const std::string &find, const std::string &replace) {
    size_t pos = 0;

    while ((pos = symbol.find(find, pos)) != std::string::npos)
        symbol.replace(pos, find.length(), replace);
}

/************************   MSVC   ************************/
#if defined(_WIN32) && defined(_MSC_VER)

static void add_space_after_comma(std::string &symbol) {
    std::string result;

    for (char c : symbol) {
        if (c == ',')
            result += ", ";
        else
            result += c;
    }

    symbol = result;
}

std::string format_symbol(std::string symbol) {
    replace_occurrences(symbol, "(class ", "(");
    replace_occurrences(symbol, "<class ", "<");
    replace_occurrences(symbol, ",class ", ",");

    replace_occurrences(symbol, "(struct ", "(");
    replace_occurrences(symbol, "<struct ", "<");
    replace_occurrences(symbol, ",struct ", ",");

    replace_occurrences(symbol, "> >", ">>");
    replace_occurrences(symbol, ">const", "> const");

    add_space_after_comma(symbol);

    return symbol;
}

#else /************************   gcc, clang, MinGW   ************************/

static void add_sym_separator(std::string &symbol, char c) {
    size_t pos = 0;

    while ((pos = symbol.find(c, pos)) != std::string::npos) {
        if (pos &&
            symbol[pos - 1] != ' ' &&
            symbol[pos - 1] != '&' &&
            symbol[pos - 1] != '*' &&
            symbol[pos - 1] != '(') {
            symbol.replace(pos, 1, std::string(" ") + c);
            pos += 2;
        } else {
            pos += 1;
        }
    }
}

std::string format_symbol(std::string symbol) {
    replace_occurrences(symbol, "std::__1::", "std::");
    replace_occurrences(symbol, "std::__cxx11::", "std::");

    replace_occurrences(symbol, "[abi:cxx11]", "");
    replace_occurrences(symbol, "[abi:ue170006]", "");

    replace_occurrences(symbol, "()", "(void)");
    replace_occurrences(symbol, "> >", ">>");

    add_sym_separator(symbol, '*');
    add_sym_separator(symbol, '&');

    return symbol;
}

#endif
