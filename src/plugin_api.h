#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <stdbool.h>

struct ufh_init_rc
{
    bool    init_successful;
    void    *context;
};

ufh_init_rc ufh_plugin_init(void);

void ufh_plugin_destroy(void *context);

bool ufh_fence_off(void *context, const char *nodename, size_t nodename_length);

bool ufh_fence_on(void *context, const char *nodename, size_t nodename_length);

bool ufh_fence_reboot(void *context, const char *nodename, size_t nodename_length);

#endif /* PLUGIN_API_H */
