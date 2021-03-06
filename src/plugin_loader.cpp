#include "plugin_loader.h"

#include "exceptions.h"

extern "C"
{
    #include <dlfcn.h>
}

namespace plugin
{
    const char* const SYMBOL_INIT           = "ufh_plugin_init";
    const char* const SYMBOL_DESTROY        = "ufh_plugin_destroy";
    const char* const SYMBOL_FENCE_OFF      = "ufh_fence_off";
    const char* const SYMBOL_FENCE_ON       = "ufh_fence_on";
    const char* const SYMBOL_FENCE_REBOOT   = "ufh_fence_reboot";

    // @throws OsException
    void* load_plugin(const char* const path, function_table& functions)
    {
        void* const plugin_handle = dlopen(path, RTLD_NOW);
        if (plugin_handle == nullptr)
        {
            throw OsException(OsException::ErrorId::DYN_LOAD_ERROR);
        }

        function_table tmp_functions;

        tmp_functions.ufh_plugin_init = reinterpret_cast<init_call> (dlsym(plugin_handle, SYMBOL_INIT));
        tmp_functions.ufh_plugin_destroy = reinterpret_cast<destroy_call> (dlsym(plugin_handle, SYMBOL_DESTROY));
        tmp_functions.ufh_fence_off = reinterpret_cast<fence_call> (dlsym(plugin_handle, SYMBOL_FENCE_OFF));
        tmp_functions.ufh_fence_on = reinterpret_cast<fence_call> (dlsym(plugin_handle, SYMBOL_FENCE_ON));
        tmp_functions.ufh_fence_reboot = reinterpret_cast<fence_call> (dlsym(plugin_handle, SYMBOL_FENCE_REBOOT));

        if (tmp_functions.ufh_plugin_init != nullptr && tmp_functions.ufh_plugin_destroy != nullptr &&
            tmp_functions.ufh_fence_off != nullptr && tmp_functions.ufh_fence_on != nullptr &&
            tmp_functions.ufh_fence_reboot != nullptr)
        {
            functions = tmp_functions;
        }
        else
        {
            throw OsException(OsException::ErrorId::DYN_LOAD_ERROR);
        }

        return plugin_handle;
    }

    void unload_plugin(void* plugin_handle, function_table& functions) noexcept
    {
        dlclose(plugin_handle);

        functions.ufh_plugin_init = nullptr;
        functions.ufh_plugin_destroy = nullptr;
        functions.ufh_fence_off = nullptr;
        functions.ufh_fence_on = nullptr;
        functions.ufh_fence_reboot = nullptr;
    }
}
