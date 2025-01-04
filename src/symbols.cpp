/**
 * @file symbols.cpp
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2025 Martin Olivier
 *
 * This library is released under MIT license
 */

#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

std::string demangle_symbol(const char *symbol);

static void add_symbol(std::vector<std::string> &result, const char *symbol, bool demangle) {
    if (!symbol || strcmp(symbol, "") == 0)
        return;

    if (demangle) {
        std::string demangled = demangle_symbol(symbol);
        if (!demangled.empty()) {
            if (std::find(result.begin(), result.end(), demangled) == result.end())
                result.push_back(demangled);
        }
    } else {
#if defined(__APPLE__)
        if (symbol[0] == '_')
            symbol++;
#endif
        if (std::find(result.begin(), result.end(), symbol) == result.end())
            result.push_back(symbol);
    }
}

/************************   Windows   ************************/
#if (defined(_WIN32) || defined(_WIN64))

#include <windows.h>
#include <tchar.h>

std::vector<std::string> get_symbols(HMODULE handle, bool demangle, bool loadable) {
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
#include <fcntl.h>
#include <unistd.h>
#include <utility>

static std::vector<std::string> get_symbols_at_off(void *handle, int fd, bool demangle, bool loadable, off_t offset, bool is_64_bit) {
    std::vector<std::string> symbols_list;
    struct mach_header_64 mh64;
    struct mach_header mh;
    off_t header_offset;
    uint32_t ncmds;

    lseek(fd, offset, SEEK_SET);

    if (is_64_bit)
        read(fd, &mh64, sizeof(mh64));
    else
        read(fd, &mh, sizeof(mh));

    ncmds = is_64_bit ? mh64.ncmds : mh.ncmds;
    header_offset = is_64_bit ? sizeof(struct mach_header_64) : sizeof(struct mach_header);

    lseek(fd, offset + header_offset, SEEK_SET);

    for (uint32_t i = 0; i < ncmds; i++) {
        struct load_command lc;
        off_t cmd_offset;

        read(fd, &lc, sizeof(lc));

        cmd_offset = lseek(fd, 0, SEEK_CUR);

        if (lc.cmd == LC_SYMTAB) {
            struct nlist_64 *symbols64 = nullptr;
            struct nlist *symbols = nullptr;
            struct symtab_command symtab;
            char *strtab;

            lseek(fd, cmd_offset - sizeof(lc), SEEK_SET);
            read(fd, &symtab, sizeof(symtab));

            if (is_64_bit) {
                symbols64 = (struct nlist_64 *)(malloc(symtab.nsyms * sizeof(struct nlist_64)));
                if (symbols64 == nullptr)
                    throw std::bad_alloc();

                lseek(fd, offset + symtab.symoff, SEEK_SET);
                read(fd, symbols64, symtab.nsyms * sizeof(struct nlist_64));
            } else {
                symbols = (struct nlist *)(malloc(symtab.nsyms * sizeof(struct nlist)));
                if (symbols == nullptr)
                    throw std::bad_alloc();

                lseek(fd, offset + symtab.symoff, SEEK_SET);
                read(fd, symbols, symtab.nsyms * sizeof(struct nlist));
            }

            strtab = (char *)(malloc(symtab.strsize));
            if (strtab == nullptr)
                throw std::bad_alloc();

            lseek(fd, offset + symtab.stroff, SEEK_SET);
            read(fd, strtab, symtab.strsize);

            for (uint32_t j = 0; j < symtab.nsyms; j++) {
                uint32_t strx;
                char *name;

                if (is_64_bit)
                    strx = symbols64[j].n_un.n_strx;
                else
                    strx = symbols[j].n_un.n_strx;

                name = &strtab[strx];

                if (!loadable || dlsym(handle, name))
                    add_symbol(symbols_list, name, demangle);
            }

            free(symbols64);
            free(symbols);
            free(strtab);
        }

        lseek(fd, cmd_offset + lc.cmdsize - sizeof(lc), SEEK_SET);
    }

    return symbols_list;
}

std::vector<std::string> get_symbols(void *handle, int fd, bool demangle, bool loadable) {
    std::vector<std::string> symbols_list;
    uint32_t magic;

    lseek(fd, 0, SEEK_SET);
    read(fd, &magic, sizeof(magic));
    lseek(fd, 0, SEEK_SET);

    if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
        struct fat_header fat_header;
        struct fat_arch *fat_arches;

        read(fd, &fat_header, sizeof(fat_header));

        fat_arches = (struct fat_arch *)(malloc(sizeof(struct fat_arch) * ntohl(fat_header.nfat_arch)));
        if (fat_arches == nullptr)
            throw std::bad_alloc();

        read(fd, fat_arches, sizeof(struct fat_arch) * ntohl(fat_header.nfat_arch));

        for (uint32_t i = 0; i < ntohl(fat_header.nfat_arch); i++) {
            std::vector<std::string> tmp = get_symbols_at_off(
                handle,
                fd,
                demangle,
                loadable,
                ntohl(fat_arches[i].offset),
                ntohl(fat_arches[i].cputype) == CPU_TYPE_X86_64);
            std::move(tmp.begin(), tmp.end(), std::back_inserter(symbols_list));
        }

        free(fat_arches);
    } else if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) {
        symbols_list = get_symbols_at_off(handle, fd, demangle, loadable, 0, true);
    } else if (magic == MH_MAGIC || magic == MH_CIGAM) {
        symbols_list = get_symbols_at_off(handle, fd, demangle, loadable, 0, false);
    } else {
        throw std::string("Unsupported file format");
    }

    return symbols_list;
}

#else /************************   Linux   ************************/

#include <libelf.h>
#include <gelf.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

std::vector<std::string> get_symbols(void *handle, int fd, bool demangle, bool loadable) {
    std::vector<std::string> result;
    Elf_Scn *scn = nullptr;
    Elf *elf;

    if (elf_version(EV_CURRENT) == EV_NONE)
        throw std::string("ELF library is out of date");

    elf = elf_begin(fd, ELF_C_READ, nullptr);
    if (!elf)
        throw std::string("elf_begin() failed");

    while ((scn = elf_nextscn(elf, scn))) {
        GElf_Shdr shdr;

        if (gelf_getshdr(scn, &shdr) == nullptr) {
            elf_end(elf);
            throw std::string("gelf_getshdr() failed");
        }

        if (shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM) {
            Elf_Data *data = elf_getdata(scn, nullptr);

            if (!data) {
                elf_end(elf);
                throw std::string("elf_getdata() failed");
            }

            for (int i = 0; i < (shdr.sh_size / shdr.sh_entsize); i++) {
                GElf_Sym sym;
                char *name;

                if (!gelf_getsym(data, i, &sym)) {
                    elf_end(elf);
                    throw std::string("gelf_getsym() failed");
                }

                name = elf_strptr(elf, shdr.sh_link, sym.st_name);
                if (!name) {
                    elf_end(elf);
                    throw std::string("elf_strptr() failed");
                }

                if (!loadable || dlsym(handle, name))
                    add_symbol(result, name, demangle);
            }
        }
    }

    elf_end(elf);

    return result;
}

#endif
