#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <stdbool.h>

bool ufh_plugin_init(void);

bool ufh_fence_off(const char *nodename, size_t nodename_length);

bool ufh_fence_on(const char *nodename, size_t nodename_length);

bool ufh_fence_reboot(const char *nodename, size_t nodename_length);

#endif /* PLUGIN_API_H */
