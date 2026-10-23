/* Loads plugin.so (C++ with its own static runtime) into a C program. */
#include <dlfcn.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    const char* path = argc > 1 ? argv[1] : "./plugin.so";
    void* plugin = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!plugin) {
        printf("dlopen %s: %s\nFAILED\n", path, dlerror());
        return 1;
    }
    int (*lookup)(const char*) = (int (*)(const char*))dlsym(plugin, "plugin_lookup");
    int ok = lookup && lookup("two") == 2 && lookup("three") == -1;
    printf("%-28s %s\n", "plugin call + exception", ok ? "ok" : "FAIL");
    printf("dlclose (expect the plugin's destructor first)\n");
    dlclose(plugin);
    printf("%s\n", ok ? "all passed" : "FAILED");
    return ok ? 0 : 1;
}
