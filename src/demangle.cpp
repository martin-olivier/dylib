#include <string>

std::string format_symbol(std::string input);

#if (defined(_WIN32) || defined(_WIN64))

#include <windows.h>
#include <dbghelp.h>
#include <tchar.h>

#pragma comment(lib, "dbghelp.lib")

std::string get_demangled_name(const char *symbol) {
    char undecorated[MAX_SYM_NAME];
    DWORD flags = UNDNAME_COMPLETE | UNDNAME_NO_FUNCTION_RETURNS | UNDNAME_NO_MS_KEYWORDS;

    // Get undecorated symbol signature
    if (UnDecorateSymbolName(symbol, undecorated, MAX_SYM_NAME, flags)) {
        std::string signature = undecorated;

        // Get undecorated symbol name
        if (UnDecorateSymbolName(symbol, undecorated, MAX_SYM_NAME, UNDNAME_NAME_ONLY)) {
            /*
             * If symbol signature starts with symbol name, it means
             * that this is a function, otherwise, this is a variable:
             * 
             * signature: tools::adder(double, double)
             * symbol name: tools::adder
             * 
             * signature: long ptr
             * symbol name: ptr
             */
            if (signature.find(undecorated) == 0)
                return format_symbol(signature);
            else
                return format_symbol(undecorated);
        }
    }

    return "";
}

#else

#include <cxxabi.h>
#include <cstring>

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

#endif