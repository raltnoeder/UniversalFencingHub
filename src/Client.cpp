#include "Client.h"
#include "version.h"

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

// @throws std::bad_alloc
Client::ExitCode Client::run()
{
    std::cout << "DEBUG: Client::run() invoked, pgm_call_path = " << pgm_call_path << std::endl;

    std::cout << "DEBUG: Invoking read_parameters()" << std::endl;
    FenceParameters params;
    read_parameters(params);
    std::cout << "DEBUG: Returned from read_parameters()" << std::endl;

    return ExitCode::FENCING_FAILURE;
}

// @throws std::bad_alloc, OsException
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
                    params.action = param_value;
                    params.have_action = true;
                }
                else
                if (param_key == KEY_IPADDR)
                {
                    params.ip_address = param_value;
                    params.have_ip_address = true;
                }
                else
                if (param_key == KEY_PORT)
                {
                    params.tcp_port = param_value;
                    params.have_tcp_port = true;
                }
                else
                if (param_key == KEY_SECRET)
                {
                    params.secret = param_value;
                    params.have_secret = true;
                }
                else
                if (param_key == KEY_NODENAME)
                {
                    params.nodename = param_value;
                    params.have_nodename = true;
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
    catch (std::bad_alloc&)
    {
        std::cerr << pgm_call_path << ": Execution failed: Out of memory" << std::endl;
    }
    return rc;
}
