#include "Client.h"
#include "version.h"
#include "client_exceptions.h"

#include <new>
#include <memory>
#include <stdexcept>
#include <iostream>

const char* const Client::DEFAULT_APP_NAME = "fence-universal";

const std::string Client::KEY_ACTION("action");
const std::string Client::KEY_IPADDR("ipaddr");
const std::string Client::KEY_PORT("port");
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
    std::cout << "DEBUG: Client::run() invoked, pgm_call_path = " << pgm_call_path << std::endl;

    std::cout << "DEBUG: Invoking read_parameters()" << std::endl;
    FenceParameters params;
    read_parameters(params);
    std::cout << "DEBUG: Returned from read_parameters()" << std::endl;

    return ExitCode::FENCING_FAILURE;
}

// @throws std::bad_alloc, OsException, ClientException
void Client::read_parameters(FenceParameters& params)
{
    std::unique_ptr<char> line_buffer_mgr(new char[LINE_BUFFER_SIZE]);
    char* const line_buffer = line_buffer_mgr.get();

    try
    {
        std::cin.exceptions(std::istream::failbit | std::istream::badbit);
        while (!std::cin.eof())
        {
            std::cin.getline(line_buffer, LINE_BUFFER_SIZE);
            const std::streamsize read_count = std::cin.gcount();
            std::streamsize split_idx = 0;
            while (split_idx < read_count && line_buffer[split_idx] != KEY_VALUE_SEPARATOR)
            {
                ++split_idx;
            }
            if (split_idx < read_count)
            {
                std::string param_key(line_buffer, split_idx);
                ++split_idx;
                std::string param_value(&(line_buffer[split_idx]), read_count - split_idx);

                std::cout << "DEBUG: Read parameter " << param_key << " = " << param_value << std::endl;

                if (param_key == KEY_ACTION)
                {
                    update_parameter(KEY_ACTION, param_value, params.action, params.have_action);
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
    catch (std::ios_base::failure&)
    {
        std::cerr << "DEBUG: std::ios_base::failure exception caught" << std::endl;
    }
}

// @throws std::bad_alloc, OsException, InetException, ClientException
void Client::dispatch_request(const FenceParameters& params)
{
    if (!params.have_all_parameters())
    {
        std::string error_msg("The following required paramters were not specified by the request:\n");
        if (!params.have_action)
        {
            error_msg += "    " + KEY_ACTION;
        }
        if (!params.have_ip_address)
        {
            error_msg += "    " + KEY_IPADDR;
        }
        if (!params.have_nodename)
        {
            error_msg += "    " + KEY_NODENAME;
        }
        if (!params.have_secret)
        {
            error_msg += "    " + KEY_SECRET;
        }
        if (!params.have_tcp_port)
        {
            error_msg += "    " + KEY_PORT;
        }
        throw ClientException(error_msg);
    }

    if (params.action == ACTION_OFF || params.action == ACTION_ON || params.action == ACTION_REBOOT)
    {
        // TODO: Invoke fencing action
    }
    else
    if (params.action == ACTION_METADATA)
    {
        // TODO: Output fencing agent meta data
    }
    else
    if (params.action == ACTION_LIST)
    {
        // TODO: Not implemented, exit with the appropriate exit code
    }
    else
    if (params.action == ACTION_MONITOR)
    {
        // TODO: Perform ping request
    }
    else
    if (params.action == ACTION_STATUS)
    {
        // TODO: Is this implemented? Check API standard on what this is supposed to do
    }
    else
    if (params.action == ACTION_START || params.action == ACTION_STOP)
    {
        // TODO: Those are effectively no-ops for this agent, exit with the appropriate exit code
    }
    else
    {
        std::string error_msg("Request for invalid action '");
        error_msg += params.action;
        error_msg += "'";
        throw ClientException(error_msg);
    }
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

bool Client::FenceParameters::have_all_parameters() const
{
    return have_action && have_ip_address && have_nodename && have_secret && have_tcp_port;
}

int main(int argc, char* argv[])
{
    int rc = static_cast<int> (Client::ExitCode::FENCING_FAILURE);
    const char* pgm_call_path = argc >= 1 ? argv[0] : Client::DEFAULT_APP_NAME;
    try
    {
        Client instance(pgm_call_path);
        Client::ExitCode instance_rc = instance.run();
        rc = static_cast<int> (instance_rc);
    }
    catch (ClientException& client_exc)
    {
        std::cerr << pgm_call_path << ": " << client_exc.what() << std::endl;
    }
    catch (std::bad_alloc&)
    {
        std::cerr << pgm_call_path << ": Execution failed: Out of memory" << std::endl;
    }
    return rc;
}
