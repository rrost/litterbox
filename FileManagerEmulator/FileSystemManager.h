#pragma once

#include "LeakDetect.h"

#include <iosfwd>
#include <string>
#include <vector>
#include <unordered_map>

#include "FileSystem.h"

namespace FileSystem
{

    class Manager
    {
    public:
        Manager();

        void process(std::istream& in);

        void output(std::ostream& in);

    private:
        struct FileSystemState
        {
            ItemPtr root;
            ItemPtr currentDir;
        };

        typedef std::vector<std::string> CommandArgs;
        typedef std::function<void(FileSystemState&, const CommandArgs&)> CommandFunction;

        void addCommand(const std::string& cmd, const CommandFunction& cmdFunc);
        void processCommand(const std::string& cmd, size_t line);

        FileSystemState state_;

        typedef std::unordered_map<std::string, CommandFunction> KnownCommands;
        KnownCommands commands_;

        friend struct CommandsImpl;
    };

}
