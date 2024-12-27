/**
 * @file demangle.cpp
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2024 Martin Olivier
 *
 * This library is released under MIT license
 */

#include <string>

std::string format_symbol(std::string input);

/************************   MSVC   ************************/
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__MINGW32__)

#include <windows.h>
#include <dbghelp.h>
#include <tchar.h>

#pragma comment(lib, "dbghelp.lib")

std::string demangle_symbol(const char *symbol) {
    DWORD sign_flags = UNDNAME_COMPLETE | UNDNAME_NO_FUNCTION_RETURNS | UNDNAME_NO_MS_KEYWORDS;
    DWORD name_flags = UNDNAME_NAME_ONLY;
    char undecorated[MAX_SYM_NAME];

    // Get undecorated symbol signature
    if (UnDecorateSymbolName(symbol, undecorated, MAX_SYM_NAME, sign_flags)) {
        std::string signature = undecorated;

        // Get undecorated symbol name
        if (UnDecorateSymbolName(symbol, undecorated, MAX_SYM_NAME, name_flags)) {
            /*
             * If symbol signature starts with symbol name, it means
             * that this is a function, otherwise, this is a variable:
             * 
             * signature: tools::adder(double, double)
             * name:      tools::adder
             * 
             * signature: long ptr
             * name:      ptr
             */
            if (signature.find(undecorated) == 0)
                return format_symbol(signature);
            else // This is a variable, no need to format it.
                return undecorated;
        }
    }

    return "";
}

#else /************************   gcc, clang, MinGW   ************************/

#include <cxxabi.h>
#include <cstring>

std::string demangle_symbol(const char *symbol) {
    size_t size = strlen(symbol);
    std::string result;
    int status;
    char *buf;
    char *res;

    buf = (char *)(malloc(size));
    if (!buf)
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

#endif
