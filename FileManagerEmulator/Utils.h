#pragma once

#include "LeakDetect.h"

#include <algorithm>
#include <string>
#include <cctype>
#include <regex>

namespace Utils
{
    static const char DriveDelimiter = ':';
    static const char DirectoryDelimiter = '\\';
    static const char ExtensionDelimiter = '.';

    inline void toLowerCase(std::string& str)
    {
        std::transform(begin(str), end(str), begin(str), &tolower);
    }

    inline void toUpperCase(std::string& str)
    {
        std::transform(begin(str), end(str), begin(str), &toupper);
    }

    inline bool equalNoCase(const std::string& lhs, const std::string& rhs)
    {
        if(lhs.size() != rhs.size()) return false;

        const auto res = std::mismatch(lhs.cbegin(), lhs.cend(), rhs.cbegin(),
            [](char c1, char c2){ return tolower(c1) == tolower(c2); });

        return res.first == lhs.cend();
    }

    inline std::regex make_regex(const char* const exp)
    {
        return std::regex(exp, std::regex::icase | std::regex::optimize);
    }

    inline bool validDriveName(const std::string& drive)
    {
        static const auto rule = make_regex("[a-z]:");
        return std::regex_match(drive, rule);
    }

    inline bool validDirectoryName(const std::string& dir)
    {
        static const auto rule = make_regex("[a-z0-9]{1,8}");
        return std::regex_match(dir, rule);
    }

    inline bool validFileName(const std::string& file)
    {
        static const auto rule = make_regex("[a-z0-9]{1,8}\\.{0,1}[a-z0-9]{0,3}");
        return std::regex_match(file, rule);
    }

    inline bool validCommandName(const std::string& cmd)
    {
        static const auto rule = make_regex("[a-z]{2,10}");
        return std::regex_match(cmd, rule);
    }

    inline std::string trimSpaces(const std::string& str)
    {
        static const auto mid = make_regex("[\\s\\t]+");
        static const auto head = make_regex("^\\s");
        static const auto trail = make_regex("\\s$");

        const auto str1 = std::regex_replace(str, mid, " ");
        const auto str2 = std::regex_replace(str1, head, "");
        return std::regex_replace(str2, trail, "");
    }

    typedef std::vector<std::string> Substrings;

    inline bool parseCommand(const std::string& command, Substrings& splittedCmd)
    {
        static const auto separator = make_regex(" ");
        const auto cmd = trimSpaces(command);

        splittedCmd.clear();
        const std::sregex_token_iterator begin(cmd.begin(), cmd.end(), separator, -1);
        const std::sregex_token_iterator end;

        std::sregex_token_iterator token = begin;
        while(token != end)
        {
            const bool first = (token == begin);
            const auto str = token->str();
            ++token;

            const bool valid = (first && validCommandName(str))
                || (!first && !str.empty());

            if(!valid) return false;

            splittedCmd.push_back(str);
        }

        return true;
    }

    inline bool parsePath(const std::string& path, Substrings& splittedPath)
    {
        if(path.empty() || path.back() == DirectoryDelimiter) return false;

        static const auto separator = make_regex("\\\\");

        splittedPath.clear();
        const std::sregex_token_iterator end;
        const std::sregex_token_iterator begin(path.begin(), path.end(), separator, -1);

        std::sregex_token_iterator token = begin;
        while(token != end)
        {
            const bool first = (token == begin);
            const auto str = token->str();
            ++token;

            const bool last = (token == end);

            const bool valid = (first && validDriveName(str))
                || (last && validFileName(str))
                || validDirectoryName(str);

            if(!valid) return false;

            splittedPath.push_back(str);
        }

        return true;
    }
}
