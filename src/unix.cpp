/**
 * @file unix.cpp
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2024 Martin Olivier
 *
 * This library is released under MIT license
 */

#include <cstring>
#include <string>
#include <cxxabi.h>

void replace_occurrences(std::string &input, const std::string &keyword, const std::string &replacement);

static void add_sym_separator(std::string &input, char symbol)
{
    size_t pos = 0;

    if (input.empty()) {
        return;
    }

    while ((pos = input.find(symbol, pos)) != std::string::npos) {
        if (pos && input[pos - 1] != ' ' && input[pos - 1] != '&' && input[pos - 1] != '*') {
            input.replace(pos, 1, std::string(" ") + symbol);
            pos += 2;
        } else {
            pos++;
        }
    }
}

std::string format_symbol(std::string input) {
    replace_occurrences(input, "std::__1::", "std::");
    replace_occurrences(input, "std::__cxx11::", "std::");

    replace_occurrences(input, "()", "(void)");

    add_sym_separator(input, '*');
    add_sym_separator(input, '&');

    return input;
}

std::string get_demangled_name(const char *symbol) {
    std::string result;
    size_t size = strlen(symbol);
    int status;
    char *buf;
    char *res;

    buf = reinterpret_cast<char *>(malloc(size));
    if (buf == NULL)
        throw std::bad_alloc();

    res = abi::__cxa_demangle(symbol, buf, &size, &status);
    if (!res) {
        free(buf);
        return "";
    }

    result = format_symbol(res);

    free(res);

    return result;
}