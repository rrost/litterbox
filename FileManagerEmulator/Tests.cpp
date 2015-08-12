#include <iostream>
#include <string>

#include "Utils.h"
#include "FileSystem.h"

namespace Tests
{
    static int caseId = 0;
    static int failedCount = 0;

    void check(int id, bool result)
    {
        if(!result)
        {
            ++failedCount;
            std::cout << "Test [" << (caseId + id) << "] failed" << std::endl;
        }
    }

    void run()
    {
        failedCount = 0;
        caseId = 0;
        {
            check(1, !Utils::validFileName(""));
            check(2, !Utils::validFileName(".a.b"));
            check(3, !Utils::validFileName("AFILE356.b13d"));
            check(4, !Utils::validFileName("AFILE3567.13d"));
            check(5, !Utils::validFileName("."));
            check(6, !Utils::validFileName("a;.b"));
            check(7, !Utils::validFileName("a.b/"));
            check(8, !Utils::validDriveName(""));
            check(9, !Utils::validDriveName("a"));
            check(10, !Utils::validDriveName("ab:"));
            check(11, !Utils::validDriveName("_:"));
            check(12, !Utils::validDriveName("A:Z"));
            check(13, !Utils::validDirectoryName(""));
            check(14, !Utils::validDirectoryName("C:"));
            check(15, !Utils::validDirectoryName("."));
            check(16, !Utils::validDirectoryName("a.b"));
            check(17, !Utils::validDirectoryName("TOOLOOONG"));
            check(18, !Utils::validDirectoryName("TOO_LONG"));
            check(19, !Utils::validDirectoryName(" WRONG"));
            check(20, !Utils::validDirectoryName("WRONG:"));
            check(21, !Utils::validDirectoryName("1.2"));
        }

        caseId = 30;
        {
            check(1, Utils::validFileName("a"));
            check(2, Utils::validFileName("a."));
            check(3, Utils::validFileName("a.b"));
            check(4, Utils::validFileName("AFILE356.b13"));
            check(5, Utils::validFileName("666.42"));
            check(6, Utils::validDriveName("C:"));
            check(7, Utils::validDriveName("Z:"));
            check(8, Utils::validDriveName("a:"));
            check(9, Utils::validDriveName("z:"));
            check(10, Utils::validDirectoryName("DIR1"));
            check(11, Utils::validDirectoryName("12345678"));
            check(12, Utils::validDirectoryName("0"));
        }

        caseId = 50;
        {
            check(1, Utils::equalNoCase("", ""));
            check(2, Utils::equalNoCase("abc", "abc"));
            check(3, Utils::equalNoCase("ABC", "ABC"));
            check(4, Utils::equalNoCase("abc", "ABC"));
            check(5, Utils::equalNoCase("AbC", "aBc"));
            check(6, Utils::equalNoCase("_AbC1", "_aBc1"));
        }

        caseId = 60;
        {
            check(1, Utils::trimSpaces("") == "");
            check(2, Utils::trimSpaces(" ") == "");
            check(3, Utils::trimSpaces("   ") == "");
            check(4, Utils::trimSpaces("\t") == "");
            check(5, Utils::trimSpaces("\t \t\t") == "");
            check(6, Utils::trimSpaces("abc") == "abc");
            check(7, Utils::trimSpaces(" a   b  c  ") == "a b c");
            check(8, Utils::trimSpaces("\t a \t  b\t  c  \td\t\t  ") == "a b c d");
        }

        caseId = 70;
        {
            Utils::Substrings path;

            check(1, !Utils::parsePath("", path));
            check(2, !Utils::parsePath("\\Dir1", path));
            check(3, !Utils::parsePath("\\Dir1\\file.txt\\", path));
            check(4, !Utils::parsePath("file.txt\\file.txt\\", path));
            check(5, !Utils::parsePath("file.txt\\", path));
            check(6, !Utils::parsePath("\\file.txt", path));
            check(7, !Utils::parsePath("a\\\\file.txt", path));
            check(8, Utils::parsePath("file.txt", path) && path.size() == 1 && path[0] == "file.txt");
            check(9, Utils::parsePath("a", path) && path.size() == 1 && path[0] == "a");
            check(10, Utils::parsePath("a\\b", path) && path.size() == 2 && path[0] == "a" && path[1] == "b");
            check(11, Utils::parsePath("C:\\Dir1\\file.txt", path) &&
                path.size() == 3 && path[0] == "C:" && path[1] == "Dir1" && path[2] == "file.txt");
            check(12, Utils::parsePath("Dir1\\file.txt", path) &&
                path.size() == 2 && path[0] == "Dir1" && path[1] == "file.txt");
        }

        caseId = 90;
        {
            Utils::Substrings cmd;

            check(1, !Utils::parseCommand("", cmd));
            check(2, !Utils::parseCommand("a", cmd));
            check(3, !Utils::parseCommand("\\cd", cmd));
            check(4, !Utils::parseCommand("cd_", cmd));
            check(5, !Utils::parseCommand("c:", cmd));
            check(6, !Utils::parseCommand("Dir1", cmd));
            check(7, !Utils::parseCommand("D!", cmd));
            check(8, !Utils::parseCommand("12", cmd));
            check(9, !Utils::parseCommand("D2", cmd));
            check(10, !Utils::parseCommand("2D", cmd));

            check(11, Utils::parseCommand("PWD", cmd) && cmd.size() == 1);
            check(12, Utils::parseCommand("CD bla bal BLAH", cmd) && cmd.size() == 4 &&
                cmd[0] == "CD" && cmd[1] == "bla" && cmd[2] == "bal" && cmd[3] == "BLAH");
            check(13, Utils::parseCommand("MD  C:\\Dir1\\Dir2\\file.txt\tDir3", cmd) &&
                cmd.size() == 3 && cmd[0] == "MD" && cmd[1] == "C:\\Dir1\\Dir2\\file.txt" &&
                cmd[2] == "Dir3");
            check(14, Utils::parseCommand("     MD  \t bla\t bal\t BLAH\t ", cmd) &&
                cmd.size() == 4 && cmd[0] == "MD" && cmd[1] == "bla" && cmd[2] == "bal" && cmd[3] == "BLAH");
        }

        if(failedCount == 0) std::cout << "All tests passed OK" << std::endl;
        else std::cout << failedCount << " test(s) failed" << std::endl;
    }
}
