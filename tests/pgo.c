/* -fprofile-generate: the runtime finds the program's counters (an empty
 * profile still links and runs) and writes them. Built with the flag. */
#include <stdio.h>
#include <sys/stat.h>

const void *__llvm_profile_begin_data(void);
const void *__llvm_profile_end_data(void);
void __llvm_profile_set_filename(const char *);
int __llvm_profile_write_file(void);

__attribute__((noinline)) static int twice(int x) { return x + x; }

int main(void)
{
    static const char path[] = "pgo-test.profraw";
    int failures = 0;
    volatile int x = 1;
    long data = (const char *)__llvm_profile_end_data() - (const char *)__llvm_profile_begin_data();
    struct stat st;

    x = twice(x);
    failures += data <= 0;
    printf("%s: the counters are found (%ld bytes of records)\n", data > 0 ? "ok" : "FAIL", data);
    remove(path);
    __llvm_profile_set_filename(path);
    int written = __llvm_profile_write_file() == 0 && stat(path, &st) == 0 && st.st_size > data;
    failures += !written;
    printf("%s: the profile is written\n", written ? "ok" : "FAIL");
    remove(path);
    printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
