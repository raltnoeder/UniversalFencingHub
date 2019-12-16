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

    return ExitCode::FENCING_FAILURE;
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
