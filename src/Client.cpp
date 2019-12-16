#include "Client.h"
#include "version.h"
#include "client_exceptions.h"
#include "Shared.h"
#include "exceptions.h"
#include "ClientMetaData.h"

#include <new>
#include <stdexcept>
#include <iostream>

#include <CharBuffer.h>
#include <RangeException.h>

const char* const Client::DEFAULT_APP_NAME = "fence-universal";

Client::Client(const char* const pgm_call_path_ref)
{
    pgm_call_path = pgm_call_path_ref;
}

Client::~Client() noexcept
{
}

const char* Client::get_version() noexcept
{
    return ufh::VERSION_STRING;
}

uint32_t Client::get_version_code() noexcept
{
    return ufh::VERSION_CODE;
}

// @throws std::bad_alloc, ClientException, ArgumentsException
Client::ExitCode Client::run()
{
    ExitCode rc = ExitCode::FENCING_FAILURE;

    std::unique_ptr<ClientParameters> params(new ClientParameters());
    params->initialize();
    params->read_parameters();

    rc = dispatch_request(*params);

    return rc;
}

// @throws std::bad_alloc, OsException, InetException, ClientException, ArgumentsException
Client::ExitCode Client::dispatch_request(ClientParameters& params)
{
    ExitCode rc = ExitCode::FENCING_FAILURE;

    // The "action" argument is always required
    params.check_required();

    CharBuffer& action = params.get_value(ClientParameters::KEY_ACTION);
    if (action == ClientParameters::ACTION_OFF || action == ClientParameters::ACTION_ON ||
        action == ClientParameters::ACTION_REBOOT)
    {
        rc = fence_action(params) ? ExitCode::FENCING_SUCCESS : ExitCode::FENCING_FAILURE;
    }
    else
    if (action == ClientParameters::ACTION_METADATA)
    {
        output_metadata();
        rc = ExitCode::FENCING_SUCCESS;
    }
    else
    if (action == ClientParameters::ACTION_LIST)
    {
        rc = check_server_connection(params) ? ExitCode::FENCING_SUCCESS : ExitCode::FENCING_FAILURE;
    }
    else
    if (action == ClientParameters::ACTION_MONITOR)
    {
        rc = check_server_connection(params) ? ExitCode::FENCING_SUCCESS : ExitCode::FENCING_FAILURE;
    }
    else
    if (action == ClientParameters::ACTION_STATUS)
    {
        rc = check_server_connection(params) ? ExitCode::FENCING_SUCCESS : ExitCode::FENCING_FAILURE;
    }
    else
    if (action == ClientParameters::ACTION_START || action == ClientParameters::ACTION_STOP)
    {
        rc = ExitCode::FENCING_SUCCESS;
    }
    else
    {
        std::string error_msg("Request for invalid action '");
        error_msg += action.c_str();
        error_msg += "'";
        throw ClientException(error_msg);
    }

    return rc;
}

// @throws std::bad_alloc, OsException, InetException, ClientException, ArgumentsException
std::unique_ptr<ClientConnector> Client::init_connector(ClientParameters& params)
{
    CharBuffer& protocol = params.get_value(ClientParameters::KEY_PROTOCOL);
    CharBuffer& ip_address = params.get_value(ClientParameters::KEY_IP_ADDRESS);
    CharBuffer& tcp_port = params.get_value(ClientParameters::KEY_TCP_PORT);

    return std::unique_ptr<ClientConnector>(new ClientConnector(protocol, ip_address, tcp_port));
}

// @throws std::bad_alloc, OsException, InetException, ClientException, ProtocolException, ArgumentsException
bool Client::check_server_connection(ClientParameters& params)
{
    bool rc = false;

    params.mark_required(ClientParameters::KEY_PROTOCOL);
    params.mark_required(ClientParameters::KEY_IP_ADDRESS);
    params.mark_required(ClientParameters::KEY_TCP_PORT);

    params.check_required();

    std::unique_ptr<ClientConnector> connector_mgr = init_connector(params);
    ClientConnector& connector = *connector_mgr;

    connector.connect_to_server();
    rc = connector.check_connection();
    connector.disconnect_from_server();

    return rc;
}

// @throws std::bad_alloc, OsException, InetException, ClientException, ProtocolException, ArgumentsException
bool Client::fence_action(ClientParameters& params)
{
    bool rc = false;

    params.mark_required(ClientParameters::KEY_PROTOCOL);
    params.mark_required(ClientParameters::KEY_IP_ADDRESS);
    params.mark_required(ClientParameters::KEY_TCP_PORT);
    params.mark_required(ClientParameters::KEY_NODENAME);
    params.mark_required(ClientParameters::KEY_SECRET);

    params.check_required();

    std::unique_ptr<ClientConnector> connector_mgr = init_connector(params);
    ClientConnector& connector = *connector_mgr;

    connector.connect_to_server();

    CharBuffer& action = params.get_value(ClientParameters::KEY_ACTION);
    CharBuffer& nodename = params.get_value(ClientParameters::KEY_NODENAME);
    CharBuffer& secret = params.get_value(ClientParameters::KEY_SECRET);

    if (action == ClientParameters::ACTION_OFF)
    {
        rc = connector.fence_action_off(nodename, secret);
    }
    else
    if (action == ClientParameters::ACTION_ON)
    {
        rc = connector.fence_action_on(nodename, secret);
    }
    else
    if (action == ClientParameters::ACTION_REBOOT)
    {
        rc = connector.fence_action_reboot(nodename, secret);
    }
    else
    {
        std::string error_msg("Logic error: fence_action(...) called with invalid action \"");
        error_msg += action.c_str();
        error_msg += "\"";
        throw ClientException(error_msg);
    }

    connector.disconnect_from_server();

    return rc;
}

int main(int argc, char* argv[])
{
    int rc = static_cast<int> (Client::ExitCode::FENCING_FAILURE);
    const char* pgm_call_path = argc >= 1 ? argv[0] : Client::DEFAULT_APP_NAME;
    try
    {
        Client instance(pgm_call_path);
        Client::ExitCode instance_rc = instance.run();
        if (instance_rc == Client::ExitCode::FENCING_SUCCESS)
        {
            std::cout << "\x1B[1;32mAction successful\x1b[0m" << std::endl;
        }
        else
        {
            std::cout << "\x1B[1;31mAction failed\x1B[0m" << std::endl;
        }
        rc = static_cast<int> (instance_rc);
    }
    catch (Arguments::ArgumentsException& arg_exc)
    {
        std::cerr << pgm_call_path << ": Invalid arguments" << std::endl;
        std::cerr << arg_exc.what() << std::endl;
    }
    catch (ClientException& client_exc)
    {
        std::cerr << pgm_call_path << ": " << client_exc.what() << std::endl;
    }
    catch (InetException& inet_exc)
    {
        std::cerr << pgm_call_path << ": Network communication failed: " << inet_exc.what() << std::endl;
    }
    catch (OsException& os_exc)
    {
        std::cerr << pgm_call_path << ": System error: " << os_exc.what() << std::endl;
    }
    catch (std::bad_alloc&)
    {
        std::cerr << pgm_call_path << ": Execution failed: Out of memory" << std::endl;
    }
    return rc;
}

void Client::output_metadata()
{
    std::cout << metadata::CRM_META_DATA_XML << std::endl;
}
