/* __dso_handle, which GCC's crtbegin.o defines and the BB10 link line has no
 * ARM copy of. It is hidden, so every executable and shared library linked
 * with this archive gets its own: __cxa_atexit() files the destructors of a
 * module's statics under it. As crtbeginS.o does, a shared library runs them
 * when it is unloaded, before its code is unmapped; in an executable this
 * finds nothing left to run, since exit() has already run them. */
extern int __cxa_finalize(void *dso);

__attribute__((visibility("hidden"))) void *__dso_handle = &__dso_handle;

__attribute__((destructor)) static void finalize_module(void)
{
    __cxa_finalize(&__dso_handle);
}
