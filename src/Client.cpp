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

const std::string Client::KEY_ACTION("action");
const std::string Client::KEY_PROTOCOL("protocol");
const std::string Client::KEY_IPADDR("ip_address");
const std::string Client::KEY_PORT("tcp_port");
const std::string Client::KEY_SECRET("secret");
const std::string Client::KEY_NODENAME("nodename");

const std::string Client::ACTION_OFF("off");
const std::string Client::ACTION_ON("on");
const std::string Client::ACTION_REBOOT("reboot");
const std::string Client::ACTION_METADATA("metadata");
const std::string Client::ACTION_STATUS("status");
const std::string Client::ACTION_LIST("list");
const std::string Client::ACTION_MONITOR("monitor");
const std::string Client::ACTION_START("start");
const std::string Client::ACTION_STOP("stop");

const char Client::KEY_VALUE_SEPARATOR = '=';

const size_t Client::LINE_BUFFER_SIZE = 512;

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

// @throws std::bad_alloc, ClientException
Client::ExitCode Client::run()
{
    ExitCode rc = ExitCode::FENCING_FAILURE;

    FenceParameters params;
    read_parameters(params);
    rc = dispatch_request(params);

    return rc;
}

// @throws std::bad_alloc, OsException, ClientException
void Client::read_parameters(FenceParameters& params)
{
    std::unique_ptr<char[]> line_buffer_mgr(new char[LINE_BUFFER_SIZE]);
    char* const line_buffer = line_buffer_mgr.get();

    try
    {
        std::cin.exceptions(std::ios_base::badbit);
        while (std::cin.good())
        {
            std::cin.getline(line_buffer, LINE_BUFFER_SIZE);
            const std::streamsize read_count = std::cin.gcount();
            if (read_count > 1)
            {
                const std::streamsize line_length = read_count - 1;
                std::streamsize split_idx = 0;
                while (split_idx < line_length && line_buffer[split_idx] != KEY_VALUE_SEPARATOR)
                {
                    ++split_idx;
                }
                if (split_idx < line_length)
                {
                    std::string param_key(line_buffer, split_idx);
                    ++split_idx;
                    std::string param_value(&(line_buffer[split_idx]), line_length - split_idx);


                    if (param_key == KEY_ACTION)
                    {
                        update_parameter(KEY_ACTION, param_value, params.action, params.have_action);
                    }
                    else
                    if (param_key == KEY_PROTOCOL)
                    {
                        update_parameter(KEY_PROTOCOL, param_value, params.protocol, params.have_protocol);
                    }
                    else
                    if (param_key == KEY_IPADDR)
                    {
                        update_parameter(KEY_IPADDR, param_value, params.ip_address, params.have_ip_address);
                    }
                    else
                    if (param_key == KEY_PORT)
                    {
                        update_parameter(KEY_PORT, param_value, params.tcp_port, params.have_tcp_port);
                    }
                    else
                    if (param_key == KEY_SECRET)
                    {
                        update_parameter(KEY_SECRET, param_value, params.secret, params.have_secret);
                    }
                    else
                    if (param_key == KEY_NODENAME)
                    {
                        update_parameter(KEY_NODENAME, param_value, params.nodename, params.have_nodename);
                    }
                    else
                    {
                        std::cerr << "DEBUG: Unknown parameter key '" << param_key << "' ignored" << std::endl;
                    }
                }
                else
                {
                    std::cerr << "DEBUG: Invalid input line '" << line_buffer << "'" << std::endl;
                }
            }
        }
        if (std::cin.fail() && !std::cin.eof())
        {
            throw OsException(OsException::ErrorId::IO_ERROR);
        }
    }
    catch (std::ios_base::failure&)
    {
        throw OsException(OsException::ErrorId::IO_ERROR);
    }
}

// @throws std::bad_alloc, OsException, InetException, ClientException
Client::ExitCode Client::dispatch_request(const FenceParameters& params)
{
    ExitCode rc = ExitCode::FENCING_FAILURE;

    if (!params.have_action)
    {
        std::string error_msg("The required parameter \"");
        error_msg += KEY_ACTION + "\" was not specified by the request";
        throw ClientException(error_msg);
    }

    if (params.action == ACTION_OFF || params.action == ACTION_ON || params.action == ACTION_REBOOT)
    {
        check_have_all_parameters(params);
        rc = fence_action(params) ? ExitCode::FENCING_SUCCESS : ExitCode::FENCING_FAILURE;
    }
    else
    if (params.action == ACTION_METADATA)
    {
        output_metadata();
        rc = ExitCode::FENCING_SUCCESS;
    }
    else
    if (params.action == ACTION_LIST)
    {
        check_have_connection_parameters(params);
        rc = check_server_connection(params) ? ExitCode::FENCING_SUCCESS : ExitCode::FENCING_FAILURE;
    }
    else
    if (params.action == ACTION_MONITOR)
    {
        check_have_connection_parameters(params);
        rc = check_server_connection(params) ? ExitCode::FENCING_SUCCESS : ExitCode::FENCING_FAILURE;
    }
    else
    if (params.action == ACTION_STATUS)
    {
        check_have_connection_parameters(params);
        rc = check_server_connection(params) ? ExitCode::FENCING_SUCCESS : ExitCode::FENCING_FAILURE;
    }
    else
    if (params.action == ACTION_START || params.action == ACTION_STOP)
    {
        rc = ExitCode::FENCING_SUCCESS;
    }
    else
    {
        std::string error_msg("Request for invalid action '");
        error_msg += params.action;
        error_msg += "'";
        throw ClientException(error_msg);
    }

    return rc;
}

// @throws std::bad_alloc, ClientException
void Client::update_parameter(
    const std::string& param_key,
    const std::string& param_value,
    std::string& param,
    bool& have_param
)
{
    if (!have_param)
    {
        param = param_value;
        have_param = true;
    }
    else
    {
        if (param == param_value)
        {
            std::cerr << "Warning: Duplicate parameter \"" << param_key << "\"" << std::endl;
        }
        else
        {
            std::string error_msg("Conflicting duplicate parameter \"");
            error_msg += param_key;
            error_msg += "\"";
            throw ClientException(error_msg);
        }
    }
}

// @throws std::bad_alloc, OsException, InetException, ClientException
std::unique_ptr<ClientConnector> Client::init_connector(const FenceParameters& params)
{
    std::unique_ptr<CharBuffer> protocol_string_mgr(new CharBuffer(constraints::PROTOCOL_PARAM_SIZE));
    std::unique_ptr<CharBuffer> ip_addr_string_mgr(new CharBuffer(constraints::IP_ADDR_PARAM_SIZE));
    std::unique_ptr<CharBuffer> port_string_mgr(new CharBuffer(constraints::PORT_PARAM_SIZE));

    CharBuffer& protocol_string = *protocol_string_mgr;
    CharBuffer& ip_addr_string = *ip_addr_string_mgr;
    CharBuffer& port_string = *port_string_mgr;

    try
    {
        protocol_string = params.protocol.c_str();
    }
    catch (RangeException&)
    {
        std::string error_msg;
        ClientException::param_length_error_msg(error_msg, KEY_PROTOCOL, constraints::PROTOCOL_PARAM_SIZE);
        throw ClientException(error_msg);
    }

    try
    {
        ip_addr_string = params.ip_address.c_str();
    }
    catch (RangeException&)
    {
        std::string error_msg;
        ClientException::param_length_error_msg(error_msg, KEY_IPADDR, constraints::IP_ADDR_PARAM_SIZE);
        throw ClientException(error_msg);
    }

    try
    {
        port_string = params.tcp_port.c_str();
    }
    catch (RangeException&)
    {
        std::string error_msg;
        ClientException::param_length_error_msg(error_msg, KEY_PORT, constraints::PORT_PARAM_SIZE);
        throw ClientException(error_msg);
    }


    return std::unique_ptr<ClientConnector>(
        new ClientConnector(protocol_string, ip_addr_string, port_string)
    );
}

// @throws std::bad_alloc, OsException, InetException, ClientException, ProtocolException
bool Client::check_server_connection(const FenceParameters& params)
{
    bool rc = false;

    std::unique_ptr<ClientConnector> connector_mgr = init_connector(params);
    ClientConnector& connector = *connector_mgr;

    connector.connect_to_server();
    rc = connector.check_connection();
    connector.disconnect_from_server();

    return rc;
}

// @throws std::bad_alloc, OsException, InetException, ClientException, ProtocolException
bool Client::fence_action(const FenceParameters& params)
{
    bool rc = false;

    std::unique_ptr<ClientConnector> connector_mgr = init_connector(params);
    ClientConnector& connector = *connector_mgr;

    connector.connect_to_server();

    if (params.action == ACTION_OFF)
    {
        rc = connector.fence_action_off(params.nodename, params.secret);
    }
    else
    if (params.action == ACTION_ON)
    {
        rc = connector.fence_action_on(params.nodename, params.secret);
    }
    else
    if (params.action == ACTION_REBOOT)
    {
        rc = connector.fence_action_reboot(params.nodename, params.secret);
    }
    else
    {
        std::string error_msg("Logic error: fence_action(...) called with invalid action \"");
        error_msg += params.action;
        error_msg += "\"";
        throw ClientException(error_msg);
    }

    connector.disconnect_from_server();

    return rc;
}

// @throws std::bad_alloc, ClientException
void Client::check_have_connection_parameters(const FenceParameters& params)
{
    if (!params.have_connection_parameters())
    {
        std::string error_msg("The following required paramters were not specified by the request:\n");
        if (!params.have_action)
        {
            error_msg += "    " + KEY_ACTION + "\n";
        }
        if (!params.have_protocol)
        {
            error_msg += "    " + KEY_PROTOCOL + "\n";
        }
        if (!params.have_ip_address)
        {
            error_msg += "    " + KEY_IPADDR + "\n";
        }
        if (!params.have_secret)
        {
            error_msg += "    " + KEY_SECRET + "\n";
        }
        if (!params.have_tcp_port)
        {
            error_msg += "    " + KEY_PORT + "\n";
        }
        throw ClientException(error_msg);
    }
}

// @throws std::bad_alloc, ClientException
void Client::check_have_all_parameters(const FenceParameters& params)
{
    if (!params.have_all_parameters())
    {
        std::string error_msg("The following required paramters were not specified by the request:\n");
        if (!params.have_action)
        {
            error_msg += "    " + KEY_ACTION + "\n";
        }
        if (!params.have_protocol)
        {
            error_msg += "    " + KEY_PROTOCOL + "\n";
        }
        if (!params.have_ip_address)
        {
            error_msg += "    " + KEY_IPADDR + "\n";
        }
        if (!params.have_nodename)
        {
            error_msg += "    " + KEY_NODENAME + "\n";
        }
        if (!params.have_secret)
        {
            error_msg += "    " + KEY_SECRET + "\n";
        }
        if (!params.have_tcp_port)
        {
            error_msg += "    " + KEY_PORT + "\n";
        }
        throw ClientException(error_msg);
    }
}

bool Client::FenceParameters::have_all_parameters() const
{
    return have_action && have_protocol && have_ip_address && have_tcp_port && have_nodename && have_secret;
}

bool Client::FenceParameters::have_connection_parameters() const
{
    return have_action && have_protocol && have_ip_address && have_tcp_port && have_secret;
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
