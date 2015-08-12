#include "LeakDetect.h"

#include <iostream>
#include <string>

#include "Utils.h"
#include "FileSystem.h"
#include "FileSystemManager.h"

ENABLE_LEAK_DETECTION;

namespace Tests
{
    void run();
}

int main(int argc, char* argv[])
{
    if(argc == 2 && Utils::equalNoCase(argv[1], "--tests"))
    {
        Tests::run();
        return 0;
    }

    try
    {
        FileSystem::Manager manager;
        manager.process(std::cin);
        manager.output(std::cout);
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
