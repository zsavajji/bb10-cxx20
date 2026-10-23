/* <link.h> as glibc has it: QNX declares dl_iterate_phdr() and struct
 * dl_phdr_info in <sys/link.h>, without the ElfW() macro, and with a const
 * callback argument. */
#ifndef BB10_CXX20_LINK_H
#define BB10_CXX20_LINK_H

#include <sys/link.h>

#ifndef ElfW
#define ElfW(type) Elf32_##type
#endif

#ifdef __cplusplus
/* glibc's callback takes a non-const dl_phdr_info, QNX's a const one. */
extern "C++" inline int dl_iterate_phdr(int (*callback)(struct dl_phdr_info *, size_t, void *),
                                        void *data)
{
    return dl_iterate_phdr(
        reinterpret_cast<int (*)(const struct dl_phdr_info *, size_t, void *)>(callback), data);
}
#endif

#endif
