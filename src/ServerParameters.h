#ifndef SERVERPARAMETERS_H
#define SERVERPARAMETERS_H

#include <new>
#include <memory>
#include <CharBuffer.h>
#include "Arguments.h"

class ServerParameters : public Arguments
{
  public:
    static const size_t MAX_PARAMETER_SIZE;

    static const char* const KEY_PROTOCOL;
    static const char* const KEY_BIND_ADDRESS;
    static const char* const KEY_TCP_PORT;
    static const char* const KEY_FENCE_MODULE;

    static const CharBuffer OPT_PREFIX;

    // @throws std::bad_alloc
    ServerParameters();
    virtual ~ServerParameters() noexcept;
    ServerParameters(const ServerParameters& other) = delete;
    ServerParameters(ServerParameters&& orig) = delete;
    virtual ServerParameters& operator=(const ServerParameters& other) = default;
    virtual ServerParameters& operator=(ServerParameters&& orig) = default;

    // @throws std::bad_alloc
    virtual void initialize();

    // @throws std::bad_alloc, ArgumentsException
    virtual void read_parameters(int argc, const char* const argv[]);
};

#endif /* SERVERPARAMETERS_H */
