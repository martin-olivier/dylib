#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <tchar.h>

#pragma comment(lib, "dbghelp.lib")

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

void replace_occurrences(std::string &input, const std::string &keyword, const std::string &replacement);

void add_space_after_comma(std::string &input) {
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

    input.erase(
        std::remove_if(
            input.begin(),
            input.end(),
            ::isspace
        ), input.end()
    );

    add_space_after_comma(input);

    return input;
}

std::string get_demangled_name(const char *symbol) {
    char undecoratedName[MAX_SYM_NAME];
    DWORD flags = UNDNAME_COMPLETE | UNDNAME_NO_FUNCTION_RETURNS | UNDNAME_NO_MS_KEYWORDS;

    if (UnDecorateSymbolName(symbol, undecoratedName, MAX_SYM_NAME, flags))
        return format_symbol(undecoratedName);

    return "";
}

std::vector<std::string> get_symbols(HMODULE hModule, bool demangle) {
    std::vector<std::string> result;

    // Get the DOS header
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw std::string("Invalid DOS header");

    // Get the NT headers
    PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + pDosHeader->e_lfanew);
    if (pNTHeaders->Signature != IMAGE_NT_SIGNATURE)
        throw std::string("Invalid NT headers");

    // Get the export directory
    DWORD exportDirRVA = pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (exportDirRVA == 0)
        throw std::string("No export directory found");

    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule + exportDirRVA);

    // Get the list of exported function names
    DWORD* pNames = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfNames);
    DWORD* pFunctions = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfFunctions);
    WORD* pNameOrdinals = (WORD*)((BYTE*)hModule + pExportDir->AddressOfNameOrdinals);

    for (DWORD i = 0; i < pExportDir->NumberOfNames; ++i) {
        const char* name = (const char*)((BYTE*)hModule + pNames[i]);

        if (!name)
            continue;

        if (strcmp(name, "") == 0)
            continue;

        if (demangle) {
            std::string demangled = get_demangled_name(name);
            if (!demangled.empty()) {
                if (std::find(result.begin(), result.end(), name) == result.end())
                    result.push_back(demangled);
                continue;
            }
        }

        if (std::find(result.begin(), result.end(), name) != result.end())
            continue;

        result.push_back(name);
    }

    return result;
}