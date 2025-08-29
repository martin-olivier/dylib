/**
 * @file symbols.cpp
 *
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2025 Martin Olivier
 *
 * This library is released under MIT license
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

std::string demangle_symbol(const char *symbol);

enum internal_symbol_type : std::uint8_t {
    C,
    CPP,
};

struct internal_symbol_info {
    std::string name;
    std::string demangled_name;
    internal_symbol_type type;
    bool loadable;
};

static void add_symbol(std::vector<internal_symbol_info> &result, const char *symbol,
                       bool loadable) {
    internal_symbol_type type = internal_symbol_type::C;
    std::string demangled;

    if (!symbol || strcmp(symbol, "") == 0)
        return;

    demangled = demangle_symbol(symbol);
    if (demangled.empty())
        demangled = symbol;
    else
        type = internal_symbol_type::CPP;

    /*
     * In case of duplicate symbols, for example when loading a FAT binary,
     * avoid duplicates and override loadable if the previous symbol was not loadable.
     */
    for (auto &sym : result) {
        if (sym.name == symbol) {
            if (!sym.loadable)
                sym.loadable = loadable;
            return;
        }
    }

    result.push_back({symbol, demangled, type, loadable});
}

/************************   Windows   ************************/
#if defined(_WIN32)

#include <windows.h>
#include <tchar.h>

std::vector<internal_symbol_info> get_symbols(HMODULE handle, int fd) {
    std::vector<internal_symbol_info> symbols_list;
    PIMAGE_EXPORT_DIRECTORY pExportDir;
    PIMAGE_DOS_HEADER pDosHeader;
    PIMAGE_NT_HEADERS pNTHeaders;
    DWORD exportDirRVA;
    DWORD *pNames;

    // Get the DOS header
    pDosHeader = (PIMAGE_DOS_HEADER)handle;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw std::runtime_error("Invalid DOS header");

    // Get the NT headers
    pNTHeaders = (PIMAGE_NT_HEADERS)((BYTE *)handle + pDosHeader->e_lfanew);
    if (pNTHeaders->Signature != IMAGE_NT_SIGNATURE)
        throw std::runtime_error("Invalid NT headers");

    // Get the export directory
    exportDirRVA =
        pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (exportDirRVA == 0)
        throw std::runtime_error("No export directory found");

    pExportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE *)handle + exportDirRVA);

    // Get the list of exported function names
    pNames = (DWORD *)((BYTE *)handle + pExportDir->AddressOfNames);

    for (DWORD i = 0; i < pExportDir->NumberOfNames; ++i) {
        const char *name = (const char *)((BYTE *)handle + pNames[i]);

        add_symbol(symbols_list, name, !!GetProcAddress(handle, name));
    }

    return symbols_list;
}

/************************   Mac OS   ************************/
#elif defined(__APPLE__)

#include <dlfcn.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <unistd.h>
#include <utility>

#if INTPTR_MAX == INT32_MAX
using mach_header_arch = mach_header;
using nlist_arch = nlist;
#define DYLIB_MH_MAGIC MH_MAGIC
#define DYLIB_MH_CIGAM MH_CIGAM
#elif INTPTR_MAX == INT64_MAX
using mach_header_arch = mach_header_64;
using nlist_arch = nlist_64;
#define DYLIB_MH_MAGIC MH_MAGIC_64
#define DYLIB_MH_CIGAM MH_CIGAM_64
#else
#error "Environment not 32 or 64-bit."
#endif

static void get_symbols_at_off(std::vector<internal_symbol_info> &symbols_list, void *handle,
                               int fd, off_t offset) {
    mach_header_arch mh;
    uint32_t ncmds;

    lseek(fd, offset, SEEK_SET);

    read(fd, &mh, sizeof(mh));

    ncmds = mh.ncmds;

    lseek(fd, offset + sizeof(mach_header_arch), SEEK_SET);

    for (uint32_t i = 0; i < ncmds; i++) {
        struct load_command lc;
        off_t cmd_offset;

        read(fd, &lc, sizeof(lc));

        cmd_offset = lseek(fd, 0, SEEK_CUR);

        if (lc.cmd == LC_SYMTAB) {
            std::vector<nlist_arch> symbols;
            std::vector<char> strtab;
            struct symtab_command symtab;

            lseek(fd, cmd_offset - sizeof(lc), SEEK_SET);
            read(fd, &symtab, sizeof(symtab));

            symbols.resize(symtab.nsyms);

            lseek(fd, offset + symtab.symoff, SEEK_SET);
            read(fd, symbols.data(), symtab.nsyms * sizeof(nlist_arch));

            strtab.resize(symtab.strsize);

            lseek(fd, offset + symtab.stroff, SEEK_SET);
            read(fd, strtab.data(), symtab.strsize);

            for (uint32_t j = 0; j < symtab.nsyms; j++) {
                uint32_t strx;
                char *name;

                strx = symbols[j].n_un.n_strx;
                name = &strtab[strx];

                if (name[0] == '_')
                    name++;

                add_symbol(symbols_list, name, !!dlsym(handle, name));
            }
        }

        lseek(fd, cmd_offset + lc.cmdsize - sizeof(lc), SEEK_SET);
    }
}

std::vector<internal_symbol_info> get_symbols(void *handle, int fd) {
    std::vector<internal_symbol_info> symbols_list;
    uint32_t magic;

    lseek(fd, 0, SEEK_SET);
    read(fd, &magic, sizeof(magic));
    lseek(fd, 0, SEEK_SET);

    if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
        std::vector<struct fat_arch> fat_arches;
        struct fat_header fat_header;

        read(fd, &fat_header, sizeof(fat_header));

        fat_arches.resize(ntohl(fat_header.nfat_arch));

        read(fd, fat_arches.data(), sizeof(struct fat_arch) * ntohl(fat_header.nfat_arch));

        for (uint32_t i = 0; i < ntohl(fat_header.nfat_arch); i++) {
            off_t off = ntohl(fat_arches[i].offset);

            get_symbols_at_off(symbols_list, handle, fd, off);
        }
    } else if (magic == DYLIB_MH_MAGIC || magic == DYLIB_MH_CIGAM) {
        get_symbols_at_off(symbols_list, handle, fd, 0);
    } else {
        throw std::runtime_error("Unsupported file format");
    }

    return symbols_list;
}

#else /************************   Linux   ************************/

#include <dlfcn.h>
#include <elf.h>
#include <link.h>

#if INTPTR_MAX == INT32_MAX
using ElfSym = Elf32_Sym;
#define DYLIB_ELF_ST_TYPE ELF32_ST_TYPE
#elif INTPTR_MAX == INT64_MAX
using ElfSym = Elf64_Sym;
#define DYLIB_ELF_ST_TYPE ELF64_ST_TYPE
#else
#error "Environment not 32 or 64-bit."
#endif

std::vector<internal_symbol_info> get_symbols(void *handle, int fd) {
    std::vector<internal_symbol_info> symbols_list;
    struct link_map *map = nullptr;
    unsigned long symentries = 0;
    ElfSym *symtab = nullptr;
    char *strtab = nullptr;
    unsigned long size = 0;

    if (dlinfo(handle, RTLD_DI_LINKMAP, static_cast<void *>(&map)) != 0) {
        const char *error = dlerror();
        throw std::runtime_error("dlinfo failed: " +
                                 std::string(error ? error : "Unknown error (dlerror failed)"));
    }

    for (auto *section = map->l_ld; section->d_tag != DT_NULL; ++section) {
        if (section->d_tag == DT_SYMTAB)
            symtab = (ElfSym *)section->d_un.d_ptr;
        else if (section->d_tag == DT_STRTAB)
            strtab = (char *)section->d_un.d_ptr;
        else if (section->d_tag == DT_SYMENT)
            symentries = section->d_un.d_val;
    }

    if (!symtab || !strtab || symentries == 0)
        return symbols_list;

    size = strtab - (char *)symtab;

    for (int i = 0; i < size / symentries; ++i) {
        ElfSym *sym = &symtab[i];

        if (DYLIB_ELF_ST_TYPE(symtab[i].st_info) == STT_FUNC) {
            const char *name = &strtab[sym->st_name];

            add_symbol(symbols_list, name, !!dlsym(handle, name));
        }
    }

    return symbols_list;
}

#endif
