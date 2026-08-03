/**
 * @file internal.hpp
 * @brief Declarations shared between the translation units of dylib
 *
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2025 Martin Olivier
 *
 * This library is released under MIT license
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define DYLIB_INTERNAL_UNDEFINE_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#define DYLIB_INTERNAL_UNDEFINE_NOMINMAX
#endif
#include <windows.h>
#ifdef DYLIB_INTERNAL_UNDEFINE_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef DYLIB_INTERNAL_UNDEFINE_LEAN_AND_MEAN
#endif
#ifdef DYLIB_INTERNAL_UNDEFINE_NOMINMAX
#undef NOMINMAX
#undef DYLIB_INTERNAL_UNDEFINE_NOMINMAX
#endif
#endif

/*
 * The implementation details deliberately live in 'dylib_detail' rather than in
 * a nested 'dylib' namespace: on macOS, <mach-o/loader.h> declares a global
 * 'struct dylib', which cannot coexist with a global namespace of the same name.
 */
namespace dylib_detail {

#ifdef _WIN32
using native_handle_type = HMODULE;
#else
using native_handle_type = void *;
#endif

enum symbol_type : std::uint8_t {
    C,
    CPP,
};

struct symbol_info {
    std::string name;
    std::string demangled_name;
    symbol_type type;
    bool loadable;
};

/**
 *  Collects the symbols exported by a loaded dynamic library
 *
 *  @throws std::runtime_error if the library image could not be parsed
 *
 *  @param handle the handle of the loaded library
 *  @param fd a read only file descriptor on the library file, or -1 on the
 *  platforms whose implementation reads the image from memory
 *
 *  @return the list of symbols, without duplicates
 */
std::vector<symbol_info> get_symbols(native_handle_type handle, int fd);

/**
 *  Collects the section names of a loaded dynamic library
 *
 *  @throws std::runtime_error if the library image could not be parsed
 *
 *  @param handle the handle of the loaded library
 *  @param fd a read only file descriptor on the library file, or -1 on the
 *  platforms whose implementation reads the image from memory
 *
 *  @return the list of section names, without duplicates
 */
std::vector<std::string> get_sections(native_handle_type handle, int fd);

/**
 *  Demangles a symbol name
 *
 *  @param symbol the raw symbol name
 *
 *  @return the demangled signature, or an empty string if the symbol is not
 *  a mangled C++ symbol
 */
std::string demangle_symbol(const char *symbol);

/**
 *  Normalizes a demangled symbol signature so that it is stable across
 *  compilers and standard library implementations
 *
 *  @param symbol the demangled signature
 *
 *  @return the normalized signature
 */
std::string format_symbol(std::string symbol);

} // namespace dylib_detail
