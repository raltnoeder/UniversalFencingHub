#ifndef SERVERPARAMETERS_H
#define SERVERPARAMETERS_H

#include <CharBuffer.h>

class ServerParameters
{
  public:
    static const size_t MAX_KEY_SIZE;
    static const size_t MAX_PARAMETER_SIZE;

    static const char* const KEY_PROTOCOL;
    static const char* const KEY_BIND_ADDRESS;
    static const char* const KEY_PORT;
    static const char* const KEY_FENCE_MODULE;

    static const CharBuffer OPT_PREFIX;
    static const char KEY_VALUE_SPLIT_CHAR;

    // @throws std::bad_alloc
    ServerParameters();
    virtual ~ServerParameters() noexcept;
    ServerParameters(const ServerParameters& other) = delete;
    ServerParameters(ServerParameters&& orig) = delete;
    virtual ServerParameters& operator=(const ServerParameters& other) = default;
    virtual ServerParameters& operator=(ServerParameters&& orig) = default;

    // @throws std::bad_alloc, ServerException
    virtual void read_parameters(int argc, const char* const argv[]);

    // @throws std::bad_alloc, ServerException
    virtual CharBuffer& get_protocol();

    // @throws std::bad_alloc, ServerException
    virtual CharBuffer& get_bind_address();

    // @throws std::bad_alloc, ServerException
    virtual CharBuffer& get_port();

    // @throws std::bad_alloc, ServerException
    virtual CharBuffer& get_fence_module();

  private:
    CharBuffer  protocol;
    bool        have_protocol = false;

    CharBuffer  bind_address;
    bool        have_bind_address = false;

    CharBuffer  port;
    bool        have_port = false;

    CharBuffer  fence_module;
    bool        have_fence_module = false;
};

#endif /* SERVERPARAMETERS_H */
