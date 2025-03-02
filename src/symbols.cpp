/**
 * @file symbols.cpp
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2025 Martin Olivier
 *
 * This library is released under MIT license
 */

#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

std::string demangle_symbol(const char *symbol);

static void add_symbol(std::vector<std::string> &result, const char *symbol, bool demangle) {
    std::string final_symbol = symbol;

    if (!symbol || strcmp(symbol, "") == 0)
        return;

    if (demangle) {
        std::string demangled = demangle_symbol(symbol);
        if (!demangled.empty())
            final_symbol = demangled;
    }

    if (std::find(result.begin(), result.end(), final_symbol) == result.end())
        result.push_back(final_symbol);
}

/************************   Windows   ************************/
#if defined(_WIN32)

#include <windows.h>
#include <tchar.h>

std::vector<std::string> get_symbols(HMODULE handle, int fd, bool demangle, bool loadable) {
    std::vector<std::string> symbols_list;
    PIMAGE_EXPORT_DIRECTORY pExportDir;
    PIMAGE_DOS_HEADER pDosHeader;
    PIMAGE_NT_HEADERS pNTHeaders;
    DWORD exportDirRVA;
    DWORD *pNames;

    // Get the DOS header
    pDosHeader = (PIMAGE_DOS_HEADER)handle;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw std::string("Invalid DOS header");

    // Get the NT headers
    pNTHeaders = (PIMAGE_NT_HEADERS)((BYTE *)handle + pDosHeader->e_lfanew);
    if (pNTHeaders->Signature != IMAGE_NT_SIGNATURE)
        throw std::string("Invalid NT headers");

    // Get the export directory
    exportDirRVA = pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (exportDirRVA == 0)
        throw std::string("No export directory found");

    pExportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE *)handle + exportDirRVA);

    // Get the list of exported function names
    pNames = (DWORD *)((BYTE *)handle + pExportDir->AddressOfNames);

    for (DWORD i = 0; i < pExportDir->NumberOfNames; ++i) {
        const char *name = (const char *)((BYTE *)handle + pNames[i]);

        if (!loadable || GetProcAddress(handle, name))
            add_symbol(symbols_list, name, demangle);
    }

    return symbols_list;
}

/************************   Mac OS   ************************/
#elif defined(__APPLE__)

#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/fat.h>
#include <dlfcn.h>
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

static void get_symbols_at_off(std::vector<std::string> &symbols_list, void *handle, int fd,
                               bool demangle, bool loadable, off_t offset) {
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

                if (!loadable || dlsym(handle, name))
                    add_symbol(symbols_list, name, demangle);
            }
        }

        lseek(fd, cmd_offset + lc.cmdsize - sizeof(lc), SEEK_SET);
    }
}

std::vector<std::string> get_symbols(void *handle, int fd, bool demangle, bool loadable) {
    std::vector<std::string> symbols_list;
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

            get_symbols_at_off(symbols_list, handle, fd, demangle, loadable, off);
        }
    } else if (magic == DYLIB_MH_MAGIC || magic == DYLIB_MH_CIGAM) {
        get_symbols_at_off(symbols_list, handle, fd, demangle, loadable, 0);
    } else {
        throw std::string("Unsupported file format");
    }

    return symbols_list;
}

#else /************************   Linux   ************************/

#include <stdint.h>
#include <dlfcn.h>
#include <link.h>
#include <elf.h>

#if INTPTR_MAX == INT32_MAX
    using ElfSym = Elf32_Sym;
    #define ELF_ST_TYPE ELF32_ST_TYPE
#elif INTPTR_MAX == INT64_MAX
    using ElfSym = Elf64_Sym;
    #define ELF_ST_TYPE ELF64_ST_TYPE
#else
    #error "Environment not 32 or 64-bit."
#endif

std::vector<std::string> get_symbols(void *handle, int fd, bool demangle, bool loadable) {
    std::vector<std::string> result;
    struct link_map *map = nullptr;
    ElfSym *symtab = nullptr;
    char *strtab = nullptr;
    int symentries = 0;
    int size = 0;

    if (dlinfo(handle, RTLD_DI_LINKMAP, &map) != 0)
        throw std::string("dlinfo failed: ") + dlerror();

    for (auto section = map->l_ld; section->d_tag != DT_NULL; ++section) {
        if (section->d_tag == DT_SYMTAB)
            symtab = (ElfSym *)section->d_un.d_ptr;
        else if (section->d_tag == DT_STRTAB)
            strtab = (char*)section->d_un.d_ptr;
        else if (section->d_tag == DT_SYMENT)
            symentries = section->d_un.d_val;
    }

    if (!symtab || !strtab || symentries == 0)
        return result;

    size = strtab - (char *)symtab;

    for (int i = 0; i < size / symentries; ++i) {
        ElfSym* sym = &symtab[i];

        if (ELF_ST_TYPE(symtab[i].st_info) == STT_FUNC) {
            const char *name = &strtab[sym->st_name];

            if (!loadable || dlsym(handle, name))
                add_symbol(result, name, demangle);
        }
    }

    return result;
}

#endif
