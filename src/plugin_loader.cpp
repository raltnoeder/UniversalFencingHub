#include "plugin_loader.h"

#include "exceptions.h"

extern "C"
{
    #include <dlfcn.h>
}

namespace plugin
{
    const char* const SYMBOL_INIT       = "ufh_plugin_init";
    const char* const SYMBOL_POWER_OFF  = "ufh_power_off";
    const char* const SYMBOL_POWER_ON   = "ufh_power_on";
    const char* const SYMBOL_REBOOT     = "ufh_reboot";

    // @throws OsException
    void load_plugin(const char* const path, function_table& functions)
    {
        void* const plugin_handle = dlopen(path, RTLD_NOW);
        if (plugin_handle == nullptr)
        {
            throw OsException(OsException::ErrorId::DYN_LOAD_ERROR);
        }

        function_table tmp_functions;

        tmp_functions.ufh_plugin_init = reinterpret_cast<init_call> (dlsym(plugin_handle, SYMBOL_INIT));
        tmp_functions.ufh_power_off = reinterpret_cast<fence_call> (dlsym(plugin_handle, SYMBOL_POWER_OFF));
        tmp_functions.ufh_power_on = reinterpret_cast<fence_call> (dlsym(plugin_handle, SYMBOL_POWER_ON));
        tmp_functions.ufh_reboot = reinterpret_cast<fence_call> (dlsym(plugin_handle, SYMBOL_REBOOT));

        if (tmp_functions.ufh_plugin_init != nullptr && tmp_functions.ufh_power_off != nullptr &&
            tmp_functions.ufh_power_on != nullptr && tmp_functions.ufh_reboot != nullptr)
        {
            functions = tmp_functions;
        }
        else
        {
            throw OsException(OsException::ErrorId::DYN_LOAD_ERROR);
        }
    }
}
