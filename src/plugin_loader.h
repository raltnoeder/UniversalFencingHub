#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <cstddef>

namespace plugin
{
    struct init_rc
    {
        bool    init_successful;
        void    *context;
    };

    typedef init_rc (*init_call)();
    typedef void (*destroy_call)(void* context);
    typedef bool (*fence_call)(void* context, const char* nodename, size_t nodename_length);

    extern const char* const SYMBOL_INIT;
    extern const char* const SYMBOL_FENCE_OFF;
    extern const char* const SYMBOL_FENCE_ON;
    extern const char* const SYMBOL_FENCE_REBOOT;

    struct function_table
    {
        init_call       ufh_plugin_init     = nullptr;
        destroy_call    ufh_plugin_destroy  = nullptr;
        fence_call      ufh_fence_off       = nullptr;
        fence_call      ufh_fence_on        = nullptr;
        fence_call      ufh_fence_reboot    = nullptr;
    };

    // @throws OsException
    void load_plugin(const char* const path, function_table& functions);
}

#endif /* PLUGIN_LOADER_H */
