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
#ifdef _WIN32

#include <windows.h>
#include <tchar.h>

static PIMAGE_NT_HEADERS get_nt_headers(HMODULE handle) {
    PIMAGE_DOS_HEADER pDosHeader;
    PIMAGE_NT_HEADERS pNTHeaders;

    pDosHeader = (PIMAGE_DOS_HEADER)handle;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        throw std::runtime_error("Invalid DOS header");

    pNTHeaders = (PIMAGE_NT_HEADERS)((BYTE *)handle + pDosHeader->e_lfanew);
    if (pNTHeaders->Signature != IMAGE_NT_SIGNATURE)
        throw std::runtime_error("Invalid NT headers");

    return pNTHeaders;
}

std::vector<internal_symbol_info> get_symbols(HMODULE handle, int fd) {
    std::vector<internal_symbol_info> symbols_list;
    PIMAGE_EXPORT_DIRECTORY pExportDir;
    PIMAGE_NT_HEADERS pNTHeaders;
    DWORD exportDirRVA;
    DWORD *pNames;

    pNTHeaders = get_nt_headers(handle);

    exportDirRVA =
        pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (exportDirRVA == 0)
        throw std::runtime_error("No export directory found");

    pExportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE *)handle + exportDirRVA);

    pNames = (DWORD *)((BYTE *)handle + pExportDir->AddressOfNames);

    for (DWORD i = 0; i < pExportDir->NumberOfNames; ++i) {
        const char *name = (const char *)((BYTE *)handle + pNames[i]);

        add_symbol(symbols_list, name, !!GetProcAddress(handle, name));
    }

    return symbols_list;
}

std::vector<std::string> get_sections(HMODULE handle, int fd) {
    std::vector<std::string> sections_list;
    PIMAGE_SECTION_HEADER pSectionHeader;
    PIMAGE_NT_HEADERS pNTHeaders;
    size_t sectionHeadersOffset;
    WORD numSections;

    pNTHeaders = get_nt_headers(handle);

    numSections = pNTHeaders->FileHeader.NumberOfSections;
    sectionHeadersOffset =
        sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + pNTHeaders->FileHeader.SizeOfOptionalHeader;
    pSectionHeader = (PIMAGE_SECTION_HEADER)((BYTE *)pNTHeaders + sectionHeadersOffset);

    for (WORD i = 0; i < numSections; ++i) {
        char name[IMAGE_SIZEOF_SHORT_NAME + 1];
        size_t len = 0;

        while (len < IMAGE_SIZEOF_SHORT_NAME && pSectionHeader[i].Name[len] != '\0')
            len++;

        memcpy(name, pSectionHeader[i].Name, len);
        name[len] = '\0';
        sections_list.emplace_back(name);
    }

    return sections_list;
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
using segment_command_arch = segment_command;
#define DYLIB_MH_MAGIC MH_MAGIC
#define DYLIB_MH_CIGAM MH_CIGAM
#define DYLIB_LC_SEGMENT LC_SEGMENT
#elif INTPTR_MAX == INT64_MAX
using mach_header_arch = mach_header_64;
using nlist_arch = nlist_64;
using segment_command_arch = segment_command_64;
#define DYLIB_MH_MAGIC MH_MAGIC_64
#define DYLIB_MH_CIGAM MH_CIGAM_64
#define DYLIB_LC_SEGMENT LC_SEGMENT_64
#else
#error "Environment not 32 or 64-bit."
#endif

class lseek_error : public std::runtime_error {
public:
    lseek_error() : std::runtime_error("lseek() failed") {}
};

class read_error : public std::runtime_error {
public:
    read_error() : std::runtime_error("read() failed") {}
};

template <typename F, typename Ctx>
static void for_each_mach_load_command(int fd, off_t offset, F &&fn, Ctx ctx) {
    mach_header_arch mh;

    if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
        throw lseek_error();
    if (read(fd, &mh, sizeof(mh)) != (ssize_t)(sizeof(mh)))
        throw read_error();

    for (uint32_t i = 0; i < mh.ncmds; ++i) {
        struct load_command lc;
        off_t start;

        start = lseek(fd, 0, SEEK_CUR);
        if (start == (off_t)-1)
            throw lseek_error();

        if (read(fd, &lc, sizeof(lc)) != (ssize_t)(sizeof(lc)))
            throw read_error();

        if (lseek(fd, start, SEEK_SET) == (off_t)-1)
            throw lseek_error();

        fn(lc, fd, offset, ctx);

        if (lseek(fd, start + (off_t)(lc.cmdsize), SEEK_SET) == (off_t)-1)
            throw lseek_error();
    }
}

template <typename F, typename Ctx>
static void for_each_mach_slice(int fd, F &&fn, Ctx ctx) {
    uint32_t magic;

    if (lseek(fd, 0, SEEK_SET) == (off_t)-1)
        throw lseek_error();
    if (read(fd, &magic, sizeof(magic)) != (ssize_t)(sizeof(magic)))
        throw read_error();
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1)
        throw lseek_error();

    if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
        struct fat_header fat_header;

        if (read(fd, &fat_header, sizeof(fat_header)) != (ssize_t)(sizeof(fat_header)))
            throw read_error();

        uint32_t nfat_arch = ntohl(fat_header.nfat_arch);
        std::vector<struct fat_arch> fat_arches(nfat_arch);

        if (read(fd, fat_arches.data(), sizeof(struct fat_arch) * nfat_arch) !=
            (ssize_t)(sizeof(struct fat_arch) * nfat_arch))
            throw read_error();

        for (uint32_t i = 0; i < nfat_arch; i++)
            fn(fd, (off_t)ntohl(fat_arches[i].offset), ctx);
    } else if (magic == DYLIB_MH_MAGIC || magic == DYLIB_MH_CIGAM) {
        fn(fd, (off_t)0, ctx);
    } else {
        throw std::runtime_error("Unsupported file format");
    }
}

struct mach_symbols_context {
    std::vector<internal_symbol_info> *symbols_list;
    void *handle;
};

struct mach_sections_context {
    std::vector<std::string> *sections_list;
};

static void process_load_command_symbols(const load_command &lc, int fd, off_t offset,
                                         mach_symbols_context ctx) {
    std::vector<nlist_arch> symbols;
    struct symtab_command symtab;
    std::vector<char> strtab;
    size_t symbols_size;

    if (lc.cmd != LC_SYMTAB)
        return;

    if (read(fd, &symtab, sizeof(symtab)) != (ssize_t)(sizeof(symtab)))
        throw read_error();

    symbols.resize(symtab.nsyms);

    if (lseek(fd, offset + (off_t)(symtab.symoff), SEEK_SET) == (off_t)-1)
        throw lseek_error();

    symbols_size = symtab.nsyms * sizeof(nlist_arch);
    if (read(fd, symbols.data(), symbols_size) != (ssize_t)(symbols_size))
        throw read_error();

    strtab.resize(symtab.strsize);

    if (lseek(fd, offset + (off_t)(symtab.stroff), SEEK_SET) == (off_t)-1)
        throw lseek_error();
    if (read(fd, strtab.data(), symtab.strsize) != (ssize_t)(symtab.strsize))
        throw read_error();

    for (uint32_t i = 0; i < symtab.nsyms; i++) {
        uint32_t strx;
        char *name;

        strx = symbols[i].n_un.n_strx;
        name = &strtab[strx];

        if (name[0] == '_')
            name++;

        add_symbol(*ctx.symbols_list, name, !!dlsym(ctx.handle, name));
    }
}

static void process_mach_slice_symbols(int fd, off_t offset, mach_symbols_context ctx) {
    for_each_mach_load_command(fd, offset, process_load_command_symbols, ctx);
}

std::vector<internal_symbol_info> get_symbols(void *handle, int fd) {
    std::vector<internal_symbol_info> symbols_list;
    mach_symbols_context ctx{&symbols_list, handle};

    for_each_mach_slice(fd, process_mach_slice_symbols, ctx);

    return symbols_list;
}

static void process_load_command_sections(const load_command &lc, int fd, off_t offset,
                                          mach_sections_context ctx) {
    size_t mach_sect_name_size = 16;
    segment_command_arch seg;

    if (lc.cmd != DYLIB_LC_SEGMENT)
        return;

    if (read(fd, &seg, sizeof(seg)) != (ssize_t)(sizeof(seg)))
        throw read_error();

    seg.segname[mach_sect_name_size - 1] = '\0';

    for (const auto &s : *ctx.sections_list) {
        if (s == seg.segname)
            return;
    }

    ctx.sections_list->push_back(seg.segname);
}

static void process_mach_slice_sections(int fd, off_t offset, mach_sections_context ctx) {
    for_each_mach_load_command(fd, offset, process_load_command_sections, ctx);
}

std::vector<std::string> get_sections(void *handle, int fd) {
    std::vector<std::string> sections_list;
    mach_sections_context ctx{&sections_list};

    for_each_mach_slice(fd, process_mach_slice_sections, ctx);

    return sections_list;
}

#else /************************   Linux   ************************/

#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <unistd.h>

#if INTPTR_MAX == INT32_MAX
using ElfSym = Elf32_Sym;
using ElfEhdr = Elf32_Ehdr;
using ElfShdr = Elf32_Shdr;
#define DYLIB_ELF_ST_TYPE ELF32_ST_TYPE
#elif INTPTR_MAX == INT64_MAX
using ElfSym = Elf64_Sym;
using ElfEhdr = Elf64_Ehdr;
using ElfShdr = Elf64_Shdr;
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

std::vector<std::string> get_sections(void *handle, int fd) {
    std::vector<std::string> sections_list;
    struct link_map *map = nullptr;
    std::vector<ElfShdr> shdrs;
    std::vector<char> shstrtab;
    ElfEhdr ehdr;

    if (lseek(fd, 0, SEEK_SET) == (off_t)-1)
        throw std::runtime_error("Could not seek to beginning of file");

    if (read(fd, &ehdr, sizeof(ehdr)) != static_cast<ssize_t>(sizeof(ehdr)))
        throw std::runtime_error("Could not read ELF header");

    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3)
        throw std::runtime_error("Invalid ELF magic");

    if (ehdr.e_shnum == 0 || ehdr.e_shstrndx == SHN_UNDEF)
        return sections_list;

    shdrs.resize(static_cast<size_t>(ehdr.e_shnum));
    if (lseek(fd, static_cast<off_t>(ehdr.e_shoff), SEEK_SET) < 0)
        throw std::runtime_error("Could not seek to section headers");

    if (read(fd, shdrs.data(), sizeof(ElfShdr) * ehdr.e_shnum) !=
        static_cast<ssize_t>(sizeof(ElfShdr) * ehdr.e_shnum))
        throw std::runtime_error("Could not read section headers");

    if (ehdr.e_shstrndx >= ehdr.e_shnum)
        throw std::runtime_error("Invalid section name string table index");

    shstrtab.resize(static_cast<size_t>(shdrs[ehdr.e_shstrndx].sh_size));
    if (lseek(fd, static_cast<off_t>(shdrs[ehdr.e_shstrndx].sh_offset), SEEK_SET) < 0)
        throw std::runtime_error("Could not seek to section name string table");

    if (read(fd, shstrtab.data(), shstrtab.size()) != static_cast<ssize_t>(shstrtab.size()))
        throw std::runtime_error("Could not read section name string table");

    for (unsigned int i = 0; i < ehdr.e_shnum; i++) {
        uint32_t name_off = shdrs[i].sh_name;
        const char *name;
        size_t len = 0;

        if (name_off >= shstrtab.size())
            continue;

        name = &shstrtab[name_off];

        while (len < shstrtab.size() - name_off && name[len] != '\0')
            len++;

        if (len > 0)
            sections_list.emplace_back(name, len);
    }

    return sections_list;
}

#endif
