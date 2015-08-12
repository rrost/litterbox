#include "FileSystemManager.h"

#include <cassert>
#include <sstream>
#include <exception>
#include <stdexcept>

#include "Utils.h"

namespace FileSystem
{

    inline void raise_error(const std::string&  msg)
    {
        throw std::runtime_error(msg);
    }

    static void printItem(std::ostream& out, const ItemPtr& item, const std::string& ident,
        size_t level, bool last, bool lineToBottom, bool& dirLast)
    {
        const bool root = level == 0;

        if(dirLast) out << ident << "|" << std::endl;
        out << ident << (root ? "" : "|_") << item->name() << std::endl;

        dirLast = item->asComposite() != nullptr;
        if(!dirLast) return;

        const std::string nextIdent = root ?
            "" : (last && !lineToBottom ? ident + "   " : ident + "|   ");

        bool nextDirLast = false;
        const auto printItems = [&](const ItemPtr& p, size_t index, size_t size)
        {
            const bool last = index == size - 1;
            printItem(out, p, nextIdent, level + 1, last, lineToBottom && last, nextDirLast);
            return true;
        };

        item->asComposite()->iterate(printItems, true);
    }

    static void printTree(std::ostream& out, const ItemPtr& item)
    {
        bool dirLast = false;
        printItem(out, item, "", 0, true, true, dirLast);
    }

    struct CommandsImpl
    {
        typedef Manager::FileSystemState FileSystemState;
        typedef Manager::CommandArgs CommandArgs;
        typedef Utils::Substrings Substrings;

        template <typename Iterator>
        static Item* pathExists(Iterator begin, Iterator end, const ItemPtr& item)
        {
            assert(item);
            auto cur = item.get();

            while(begin != end)
            {
                const auto next = cur->asComposite()->findChild(*begin++);
                if(!next || (!next->asComposite() && begin != end)) return nullptr;

                cur = next;
            }

            return cur;
        }

        static Item* pathExists(FileSystemState& fs, const Substrings& path)
        {
            assert(fs.root && fs.currentDir);
            if(path.empty()) return fs.currentDir.get();

            const bool absPath = Utils::validDriveName(path.front());
            if(absPath && !Utils::equalNoCase(fs.root->name(), path.front()))
            {
                return nullptr;
            }

            const auto& startItem = absPath ? fs.root : fs.currentDir;

            // Ignore drive name for absolute path
            const auto begin = absPath ? path.cbegin() + 1 : path.cbegin();
            return pathExists(begin, path.cend(), startItem);
        }

        static void commandMD(FileSystemState& fs, const CommandArgs& args)
        {
            if(args.size() != 1) raise_error("Incorrect number of arguments");

            Substrings path;
            if(!Utils::parsePath(args.front(), path)) raise_error("Bad path format");

            assert(!path.empty());
            const auto dirName = path.back();
            path.pop_back();

            if(!Utils::validDirectoryName(dirName)) raise_error("Bad directory name");

            const auto parentDir = pathExists(fs, path);
            if(!parentDir || !parentDir->asComposite()) raise_error("Invalid path");

            const auto newDir = Item::create(ItemType::eDirectory);
            newDir->setName(dirName);

            if(!parentDir->asComposite()->addChild(newDir)) raise_error("Directory or file already exists");
        }

        static void commandCD(FileSystemState& fs, const CommandArgs& args)
        {
            if(args.size() != 1) raise_error("Incorrect number of arguments");

            Substrings path;
            if(!Utils::parsePath(args.front(), path)) raise_error("Bad path format");

            const auto newCurDir = pathExists(fs, path);
            if(!newCurDir || !newCurDir->asComposite()) raise_error("Invalid path");

            fs.currentDir = newCurDir->self();
        }

        static void commandRD(FileSystemState& fs, const CommandArgs& args)
        {
            if(args.size() != 1) raise_error("Incorrect number of arguments");

            Substrings path;
            if(!Utils::parsePath(args.front(), path)) raise_error("Bad path format");

            const auto dirToRemove = pathExists(fs, path);
            if(!dirToRemove || !dirToRemove->asComposite()) raise_error("Invalid path");

            if(!dirToRemove->deletable())
                raise_error("Unable to remove drive, current or hard-linked directory");

            if(!dirToRemove->asComposite()->empty())
                raise_error("Unable to remove non-empty directory");

            const auto parent = dirToRemove->parent().lock();
            if(!parent) raise_error("Orphaned directory (no parent)");

            const auto removed = parent->asComposite()->removeChild(*dirToRemove);
            if(!removed) raise_error("Directory not found");
        }

        static void commandDELTREE(FileSystemState& fs, const CommandArgs& args)
        {
            if(args.size() != 1) raise_error("Incorrect number of arguments");

            Substrings path;
            if(!Utils::parsePath(args.front(), path)) raise_error("Bad path format");

            const auto dirToRemove = pathExists(fs, path);
            if(!dirToRemove || !dirToRemove->asComposite()) raise_error("Invalid path");

            dirToRemove->asComposite()->removeChildren();

            // If current dir cannot be removed just silently return
            if(!dirToRemove->deletable() || !dirToRemove->asComposite()->empty()) return;

            const auto parent = dirToRemove->parent().lock();
            if(!parent) raise_error("Orphaned directory (no parent)");

            const auto removed = parent->asComposite()->removeChild(*dirToRemove);
            if(!removed) raise_error("Directory not found");
        }

        static void commandMF(FileSystemState& fs, const CommandArgs& args)
        {
            if(args.size() != 1) raise_error("Incorrect number of arguments");

            Substrings path;
            if(!Utils::parsePath(args.front(), path)) raise_error("Bad path format");

            assert(!path.empty());
            const auto fileName = path.back();
            path.pop_back();

            if(!Utils::validFileName(fileName)) raise_error("Bad file name");

            const auto parentDir = pathExists(fs, path);
            if(!parentDir) raise_error("Invalid path");

            const auto newFile = Item::create(ItemType::eFile);
            newFile->setName(fileName);

            parentDir->asComposite()->addChild(newFile);
        }

        static void commandDEL(FileSystemState& fs, const CommandArgs& args)
        {
            if(args.size() != 1) raise_error("Incorrect number of arguments");

            Substrings path;
            if(!Utils::parsePath(args.front(), path)) raise_error("Bad path format");

            assert(!path.empty());
            const auto fileToRemove = pathExists(fs, path);
            if(!fileToRemove || fileToRemove->asComposite()) raise_error("Invalid path");

            if(!fileToRemove->deletable())
                raise_error("Unable to remove hard-linked file");

            const auto parent = fileToRemove->parent().lock();
            if(!parent) raise_error("Orphaned file (no parent)");

            const auto removed = parent->asComposite()->removeChild(*fileToRemove);
            if(!removed) raise_error("File not found");
        }

        static void createLink(FileSystemState& fs, const CommandArgs& args, bool hard)
        {
            if(args.size() != 2) raise_error("Incorrect number of arguments");

            Substrings pathSrc;
            Substrings pathDst;
            if(!Utils::parsePath(args.front(), pathSrc)
                || !Utils::parsePath(args.back(), pathDst)) raise_error("Bad path format");

            assert(!pathSrc.empty() && !pathDst.empty());

            const auto source = pathExists(fs, pathSrc);
            if(!source) raise_error("Invalid source path");

            const auto targetDir = pathExists(fs, pathDst);
            if(!targetDir || !targetDir->asComposite()) raise_error("Invalid target path");

            const auto newLink = Item::create(hard ? ItemType::eHardLink : ItemType::eDynamicLink);
            if(!newLink->asLink()->linkTo(source->self())) raise_error("Source object not linkable");

            targetDir->asComposite()->addChild(newLink);
        }

        static void commandMHL(FileSystemState& fs, const CommandArgs& args)
        {
            createLink(fs, args, true);
        }

        static void commandMDL(FileSystemState& fs, const CommandArgs& args)
        {
            createLink(fs, args, false);
        }

        static void commandMOVE(FileSystemState& fs, const CommandArgs& args)
        {
            if(args.size() != 2) raise_error("Incorrect number of arguments");

            Substrings pathSrc;
            Substrings pathDst;
            if(!Utils::parsePath(args.front(), pathSrc)
                || !Utils::parsePath(args.back(), pathDst)) raise_error("Bad path format");

            assert(!pathSrc.empty() && !pathDst.empty());

            const auto source = pathExists(fs, pathSrc);
            if(!source) raise_error("Invalid source path");

            const auto targetDir = pathExists(fs, pathDst);
            if(!targetDir || !targetDir->asComposite()) raise_error("Invalid target path");

            if(!source->deletable() ||
                (source->asComposite() && !source->asComposite()->childrenDeletable()))
                raise_error("Unable to move drive, current or hard-linked directory or file");

            if(targetDir->asComposite()->findChild(source->name()))
                raise_error("Target path already contains file or directory with same name");

            const auto parent = source->parent().lock();
            if(!parent) raise_error("Orphaned file or directory (no parent)");

            const auto moved = parent->asComposite()->removeChild(*source);
            if(!moved) raise_error("File or directory not found");

            // Target dir may be invalidated in case of moving into itself.
            const auto ensureTargetDir = pathExists(fs, pathDst);
            if(!ensureTargetDir || !ensureTargetDir->asComposite())
                raise_error("Invalid target path, cannot move into itself");

            if(!ensureTargetDir->asComposite()->addChild(moved))
                raise_error("MOVE command failed, unable to move file or directory");
        }

        static void commandCOPY(FileSystemState& fs, const CommandArgs& args)
        {
            if(args.size() != 2) raise_error("Incorrect number of arguments");

            Substrings pathSrc;
            Substrings pathDst;
            if(!Utils::parsePath(args.front(), pathSrc)
                || !Utils::parsePath(args.back(), pathDst)) raise_error("Bad path format");

            assert(!pathSrc.empty() && !pathDst.empty());

            const auto source = pathExists(fs, pathSrc);
            if(!source) raise_error("Invalid source path");

            const auto targetDir = pathExists(fs, pathDst);
            if(!targetDir || !targetDir->asComposite()) raise_error("Invalid target path");

            if(targetDir->asComposite()->findChild(source->name()))
                raise_error("Target path already contains file or directory with same name");

            const auto sourceCopy = source->copy();
            if(!sourceCopy) raise_error("Source is not copyable");

            if(!targetDir->asComposite()->addChild(sourceCopy)) raise_error("Unable to copy source");
        }
    };

    Manager::Manager()
    {
        state_.root = Item::create(ItemType::eDrive);
        state_.root->setName("C:");
        state_.currentDir = state_.root;

        addCommand("md", &CommandsImpl::commandMD);
        addCommand("cd", &CommandsImpl::commandCD);
        addCommand("rd", &CommandsImpl::commandRD);
        addCommand("mf", &CommandsImpl::commandMF);
        addCommand("del", &CommandsImpl::commandDEL);
        addCommand("mhl", &CommandsImpl::commandMHL);
        addCommand("mdl", &CommandsImpl::commandMDL);
        addCommand("move", &CommandsImpl::commandMOVE);
        addCommand("copy", &CommandsImpl::commandCOPY);
        addCommand("deltree", &CommandsImpl::commandDELTREE);
    }

    void Manager::process(std::istream& in)
    {
        size_t line = 1;
        std::string cmd;
        while(std::getline(in, cmd))
        {
            if(!cmd.empty()) processCommand(cmd, line++);
        }
    }

    void Manager::output(std::ostream& out)
    {
        printTree(out, state_.root);
    }

    void Manager::addCommand(const std::string& cmd, const CommandFunction& cmdFunc)
    {
        auto cmdName = cmd;
        Utils::toLowerCase(cmdName);

        const auto found = commands_.find(cmdName);
        if(found != commands_.cend()) raise_error("Duplicate command: " + cmdName);

        commands_[cmdName] = cmdFunc;
    }

    void Manager::processCommand(const std::string& cmd, size_t line)
    {
        try
        {
            Utils::Substrings splittedCmd;
            if(!Utils::parseCommand(cmd, splittedCmd)) raise_error("Invalid command format");

            assert(!splittedCmd.empty());
            auto cmdName = splittedCmd.front();
            Utils::toLowerCase(cmdName);

            const auto found = commands_.find(cmdName);
            if(found == commands_.cend()) raise_error("Unknown command: " + cmdName);

            const auto cmdFunc = found->second;
            const CommandArgs args(splittedCmd.cbegin() + 1, splittedCmd.cend());

            cmdFunc(state_, args);
        }
        catch(std::exception& e)
        {
            std::ostringstream msg;
            msg << "Error at line " << line << ": " << e.what();
            raise_error(msg.str());
        }
    }

}
