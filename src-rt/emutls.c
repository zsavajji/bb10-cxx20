//===---------- emutls.c - __emutls_get_address for QNX 6.6 / ARMv7 -------===//
//
// A rewrite for BB10 of compiler-rt's lib/builtins/emutls.c (LLVM 19.1.7),
// from which the control structure, the slow path (index assignment, array
// growth, object allocation, the pthread key destructor) and the single
// per-process table arrangement are taken. compiler-rt's file is
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// The changes, which are this repository's (MIT, see LICENSE) over that code:
// the fast path reads the thread's array straight from QNX's thread-local
// storage block instead of through pthread_getspecific(), and the thread's
// first use of the runtime, array growth, and index assignment are out of
// line.
//
//===----------------------------------------------------------------------===//
//
// clang's -femulated-tls turns every access to a thread_local variable x into
// __emutls_get_address(&__emutls_v.x). The control structure is shared with
// GCC's and clang's code generators (that is the ABI: its layout, and this
// function's name and signature).
//
// A thread's objects are kept in an array in the thread's pthread key data,
// indexed by the number the runtime hands each variable the first time any
// thread uses it (control->object.index, 1 for the first, never 0).
//
// QNX's pthread_getspecific(key) is a libc function (<pthread.h> has an inline
// version, under __INLINE_FUNCTIONS__, which the driver does not define):
//     key < __tls()->__numkeys ? __tls()->__keydata[key] : 0
// where __tls() returns _cpupage_ptr->tls (libc.so.3's __tls() is three loads
// from the GOT-addressed _cpupage_ptr, and bx lr). <sys/syspage.h> declares
// _cpupage_ptr and struct cpupage_entry for user programs, so the fast path
// does that read itself, with no call into libc (a call through the PLT).

#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/storage.h>
#include <sys/syspage.h>

#if !defined(__QNX__) || !defined(__ARM__)
#error "emutls.c is for QNX on ARM: the fast path reads the cpupage"
#endif

typedef unsigned int gcc_word __attribute__((mode(word)));

// For every thread_local variable xyz there is one __emutls_control variable
// named __emutls_v.xyz. If xyz has a non-zero initial value, its "value" points
// to __emutls_t.xyz, which has it.
typedef struct __emutls_control {
    gcc_word size;  // size of the object in bytes
    gcc_word align; // alignment of the object in bytes
    union {
        uintptr_t index; // data[index] is the object address (0: no index yet)
        void *address;   // object address, when in a single thread environment
    } object;
    void *value; // null or non-zero initial value for the object
} __emutls_control;

_Static_assert(sizeof(__emutls_control) == 16 && sizeof(uintptr_t) == sizeof(void *),
               "__emutls_control is not the layout clang emits");

// A thread's objects. data[index] is the address of the object of the variable
// with that index; data[0] is never used, so that index 0 ("none yet") reads
// as a missing object and takes the slow path with no test of its own.
typedef struct emutls_array {
    uintptr_t skip_destructor_rounds;
    uintptr_t size; // number of elements in data[]
    void *data[];
} emutls_array;

// The pthread key of the arrays. ~0u until it is created, which no thread's
// __numkeys exceeds, so "not created yet" fails the fast path's key test
// without a test of its own.
static unsigned emutls_key = ~0u;
static pthread_mutex_t emutls_mutex = PTHREAD_MUTEX_INITIALIZER;
static uintptr_t emutls_num_object = 0; // indexes handed out; under emutls_mutex

// pthread_exit() runs the key destructors in key order, and again for as many
// rounds (PTHREAD_DESTRUCTOR_ITERATIONS, 4) as any of them stored a value
// back. Free the thread's objects in the last round: the destructors of its
// thread_local objects, which libc++abi runs from a key of its own created
// after this one, would otherwise run on freed memory, and see their
// thread_locals initialised afresh.
#define EMUTLS_SKIP_DESTRUCTOR_ROUNDS (PTHREAD_DESTRUCTOR_ITERATIONS - 1)

// Free the allocated TLS data
static void emutls_shutdown(emutls_array *array)
{
    for (uintptr_t i = 1; i < array->size; ++i) {
        // The malloc()ed address is in ((void **)object)[-1]
        if (array->data[i])
            free(((void **)array->data[i])[-1]);
    }
}

static void emutls_key_destructor(void *ptr)
{
    emutls_array *array = ptr;
    if (array->skip_destructor_rounds > 0) {
        // pthread_exit() cleared the key's value before calling us: put it
        // back, so that there is another round.
        array->skip_destructor_rounds--;
        pthread_setspecific(emutls_key, array);
    } else {
        emutls_shutdown(array);
        free(array);
    }
}

static void emutls_init(void)
{
    pthread_key_t key;
    if (pthread_key_create(&key, emutls_key_destructor) != 0)
        abort();
    __atomic_store_n(&emutls_key, key, __ATOMIC_RELEASE);
}

// Emulated TLS objects are always allocated at run-time.
static void *emutls_allocate_object(__emutls_control *control)
{
    size_t size = control->size;
    size_t align = control->align;
    if (align < sizeof(void *))
        align = sizeof(void *);
    // Make sure that align is power of 2.
    if ((align & (align - 1)) != 0)
        abort();

    char *object = malloc(align - 1 + sizeof(void *) + size);
    if (!object)
        abort();
    void *base = (void *)(((uintptr_t)(object + align - 1 + sizeof(void *))) & ~(uintptr_t)(align - 1));
    ((void **)base)[-1] = object;
    if (control->value)
        memcpy(base, control->value, size);
    else
        memset(base, 0, size);
    return base;
}

// Returns control->object.index; sets it if not allocated yet.
static uintptr_t emutls_get_index(__emutls_control *control)
{
    uintptr_t index = __atomic_load_n(&control->object.index, __ATOMIC_ACQUIRE);
    if (!index) {
        pthread_mutex_lock(&emutls_mutex);
        index = control->object.index;
        if (!index) {
            index = ++emutls_num_object;
            __atomic_store_n(&control->object.index, index, __ATOMIC_RELEASE);
        }
        pthread_mutex_unlock(&emutls_mutex);
    }
    return index;
}

// Returns the thread's array, made, or grown by realloc(), to hold index.
static emutls_array *emutls_get_array(uintptr_t index)
{
    emutls_array *array = pthread_getspecific(emutls_key);
    if (array && index < array->size)
        return array;
    // Slots for index 0 and up to it, in whole multiples of 16 words with the header.
    uintptr_t header_words = sizeof(emutls_array) / sizeof(void *);
    uintptr_t new_size = ((index + 1 + header_words + 15) & ~(uintptr_t)15) - header_words;
    uintptr_t old_size = array ? array->size : 0;
    array = realloc(array, sizeof(emutls_array) + new_size * sizeof(void *));
    if (!array)
        abort();
    if (!old_size)
        array->skip_destructor_rounds = EMUTLS_SKIP_DESTRUCTOR_ROUNDS;
    memset(array->data + old_size, 0, (new_size - old_size) * sizeof(void *));
    array->size = new_size;
    pthread_setspecific(emutls_key, array);
    return array;
}

// A thread's first use of a variable, the first use of any in the process, a
// thread's first use of the runtime, and growth of the thread's array.
__attribute__((cold, noinline)) static void *emutls_get_address_slow(__emutls_control *control)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, emutls_init); // this thread now sees emutls_key
    uintptr_t index = emutls_get_index(control);
    emutls_array *array = emutls_get_array(index);
    if (array->data[index] == NULL)
        array->data[index] = emutls_allocate_object(control);
    return array->data[index];
}

// Our emulated TLS implementation relies on local state (the pthread key), and
// if that is duplicated across shared libraries, accesses to the same
// variable from different ones yield different results. This is the only
// external entry point, and with default visibility and weak, and with
// executables exporting it (the driver links with
// --export-dynamic-symbol=__emutls_get_address), the dynamic linker coalesces
// the copies at runtime into one. As in compiler-rt: no -Bsymbolic.
//
// The fast path: the object of this variable is already allocated for this
// thread. It takes no lock and no atomic operation beyond plain loads:
//  - control->object.index and emutls_key are each read once, and each is
//    either its final value or its "not there yet" value (0 and ~0u): a word
//    written once, which a thread sees whole. A thread that sees a stale
//    "not there yet" takes the slow path, which synchronises.
//  - The numkeys test is needed, on every call. QNX grows a thread's key data
//    lazily, to the highest key the thread has set (pthread_setspecific(), and
//    realloc()s it, so it is read afresh every time too): a thread that has
//    not yet set emutls_key, even one that sees the variable's index set by
//    another thread, may have too few slots, or none, and then
//    __keydata[emutls_key] is out of bounds. Once the thread has an array,
//    numkeys never shrinks, so the test cannot fail for it, but there is no
//    way to know that before making it. The test also covers "key not
//    created yet" (above).
//  - _cpupage_ptr->tls is the current thread's block, as __tls() returns it.
//    The thread may be moved to another CPU between the loads, which is fine:
//    the kernel updates the cpupage's tls on every switch, and nothing here
//    is kept across a call.
__attribute__((visibility("default"), weak)) void *__emutls_get_address(__emutls_control *control)
{
    uintptr_t index = __atomic_load_n(&control->object.index, __ATOMIC_RELAXED);
    unsigned key = __atomic_load_n(&emutls_key, __ATOMIC_RELAXED);
    struct _thread_local_storage *tls = _cpupage_ptr->tls;
    if (__builtin_expect(key < tls->__numkeys, 1)) {
        emutls_array *array = tls->__keydata[key];
        if (__builtin_expect(array != NULL && index < array->size, 1)) {
            void *object = array->data[index];
            if (__builtin_expect(object != NULL, 1))
                return object;
        }
    }
    return emutls_get_address_slow(control);
}
