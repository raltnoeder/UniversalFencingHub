#include <new>
#include <memory>
#include <iostream>
#include "SignalHandler.h"
#include "Server.h"
#include "exceptions.h"

int main(int argc, char* argv[])
{
    int rc = EXIT_FAILURE;
    try
    {
        std::unique_ptr<SignalHandler> stop_signal(SignalHandler::get_instance());
        std::unique_ptr<Server> instance(new Server(*stop_signal));

        rc = instance->run(argc, argv);
    }
    catch (std::bad_alloc&)
    {
        std::cerr << "Initialization failed: Out of memory" << std::endl;
    }
    return rc;
}
