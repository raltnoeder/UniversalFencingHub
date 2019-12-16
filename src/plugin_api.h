#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <stdbool.h>

bool power_off(const char *nodename, size_t nodename_length);

bool power_on(const char *nodename, size_t nodename_length);

bool reboot(const char *nodename, size_t nodename_length);

#endif /* PLUGIN_API_H */
