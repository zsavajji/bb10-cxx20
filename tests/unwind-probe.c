/* What QNX tells an unwinder about where a program's exception tables are:
 * the linker's answer (__exidx_start/__exidx_end, from bb10-exidx.ld),
 * QNX's own lookup (__gnu_Unwind_Find_exidx, which BB10's unwinder uses),
 * and dl_iterate_phdr's (which LLVM's libunwind uses: load address plus
 * each segment's p_vaddr). For the program and for libc. They should agree. */
#include <link.h>
#include <stdio.h>
#include <string.h>

#define PT_ARM_EXIDX_TYPE 0x70000001

extern unsigned __gnu_Unwind_Find_exidx(unsigned pc, int *count);
extern char __exidx_start[], __exidx_end[];

static int show(const struct dl_phdr_info *info, size_t size, void *data)
{
    (void)size;
    (void)data;
    printf("%s: dlpi_addr %#x, %d headers\n", info->dlpi_name ? info->dlpi_name : "(no name)",
        (unsigned)info->dlpi_addr, info->dlpi_phnum);
    for (int i = 0; i < info->dlpi_phnum; ++i) {
        const Elf32_Phdr *phdr = &info->dlpi_phdr[i];
        if (phdr->p_type != PT_LOAD && phdr->p_type != PT_ARM_EXIDX_TYPE)
            continue;
        printf("  %-9s p_vaddr %#9x size %#8x -> at %#x\n", phdr->p_type == PT_LOAD ? "LOAD" : "ARM_EXIDX",
            (unsigned)phdr->p_vaddr, (unsigned)phdr->p_memsz, (unsigned)(info->dlpi_addr + phdr->p_vaddr));
    }
    return 0;
}

static void lookup(const char *what, unsigned pc)
{
    int count = 0;
    unsigned table = __gnu_Unwind_Find_exidx(pc, &count);
    printf("__gnu_Unwind_Find_exidx(%s at %#x): table at %#x, %d entries\n", what, pc, table, count);
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("linker: this program's table at %#x, %d entries\n", (unsigned)__exidx_start,
        (int)((__exidx_end - __exidx_start) / 8));
    lookup("main", (unsigned)&main);
    lookup("libc's strlen", (unsigned)&strlen);
    printf("dl_iterate_phdr:\n");
    dl_iterate_phdr(show, NULL);
    return 0;
}
