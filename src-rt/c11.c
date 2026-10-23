/* C11's aligned_alloc(), which BB10's libc (QNX 6.6) does not have.
 * Declared by include/bb10-compat/stdlib.h. */
#include <errno.h>
#include <stdlib.h>

void *aligned_alloc(size_t alignment, size_t size)
{
    void *p;
    int error;
    /* posix_memalign() takes multiples of sizeof(void *) only; any power of
     * two above that is also a valid smaller alignment. */
    if (alignment < sizeof(void *))
        alignment = sizeof(void *);
    error = posix_memalign(&p, alignment, size);
    if (error) {
        errno = error;
        return NULL;
    }
    return p;
}
