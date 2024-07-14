/**
 * @file linux.cpp
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2024 Martin Olivier
 *
 * This library is released under MIT license
 */

#include <stdio.h>
#include <stdlib.h>
#include <libelf.h>
#include <gelf.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <vector>
#include <string>
#include <algorithm>

std::string get_demangled_name(const char *symbol);

std::vector<std::string> get_symbols(int fd, bool demangle) {
    std::vector<std::string> result;

    if (elf_version(EV_CURRENT) == EV_NONE)
        throw std::string("ELF library initialization failed");

    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf)
        throw std::string("elf_begin() failed");

    size_t shstrndx;
    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        elf_end(elf);
        throw std::string("elf_getshdrstrndx() failed");
    }

    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            elf_end(elf);
            throw std::string("gelf_getshdr() failed");
        }

        if (shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM) {
            Elf_Data *data = NULL;
            data = elf_getdata(scn, data);
            if (!data) {
                elf_end(elf);
                throw std::string("elf_getdata() failed");
            }

            int count = shdr.sh_size / shdr.sh_entsize;
            for (int i = 0; i < count; i++) {
                GElf_Sym sym;
                if (!gelf_getsym(data, i, &sym)) {
                    elf_end(elf);
                    throw std::string("gelf_getsym() failed");
                }

                const char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);
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
        }
    }

    elf_end(elf);

    return result;
}
