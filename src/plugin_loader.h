#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <cstddef>

namespace plugin
{
    typedef bool (*init_call)();
    typedef bool (*fence_call)(const char* nodename, size_t nodename_length);

    extern const char* const SYMBOL_INIT;
    extern const char* const SYMBOL_FENCE_OFF;
    extern const char* const SYMBOL_FENCE_ON;
    extern const char* const SYMBOL_FENCE_REBOOT;

    struct function_table
    {
        init_call   ufh_plugin_init     = nullptr;
        fence_call  ufh_fence_off       = nullptr;
        fence_call  ufh_fence_on        = nullptr;
        fence_call  ufh_fence_reboot    = nullptr;
    };

    // @throws OsException
    void load_plugin(const char* const path, function_table& functions);
}

#endif /* PLUGIN_LOADER_H */
