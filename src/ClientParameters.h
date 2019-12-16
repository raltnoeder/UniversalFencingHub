#ifndef CLIENTPARAMETERS_H
#define CLIENTPARAMETERS_H

#include <CharBuffer.h>
#include "Arguments.h"

class ClientParameters : public Arguments
{
  public:
    static const char* const KEY_ACTION;
    static const char* const KEY_PROTOCOL;
    static const char* const KEY_IP_ADDRESS;
    static const char* const KEY_TCP_PORT;
    static const char* const KEY_SECRET;
    static const char* const KEY_NODENAME;

    static const char* const ACTION_OFF;
    static const char* const ACTION_ON;
    static const char* const ACTION_REBOOT;
    static const char* const ACTION_METADATA;
    static const char* const ACTION_STATUS;
    static const char* const ACTION_LIST;
    static const char* const ACTION_MONITOR;
    static const char* const ACTION_START;
    static const char* const ACTION_STOP;

    static const size_t MAX_PARAMETER_SIZE;

    ClientParameters();
    virtual ~ClientParameters() noexcept;
    ClientParameters(const ClientParameters& other) = default;
    ClientParameters(ClientParameters&& orig) = default;
    virtual ClientParameters& operator=(const ClientParameters& other) = default;
    virtual ClientParameters& operator=(ClientParameters&& orig) = default;

    // @throws std::bad_alloc
    virtual void initialize();

    // @throws std::bad_alloc, OsException, ArgumentsException
    virtual void read_parameters();
};

#endif /* CLIENTPARAMETERS_H */
