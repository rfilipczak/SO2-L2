#ifndef IOHELP_H
#define IOHELP_H

#include <vector>
#include <string>
#include <iostream>

namespace iohelp
{
    std::string get_prog_name(char** argv)
    {
        return std::string(argv[0]);
    }

    std::vector<std::string> create_arg_list(char** argv)
    {
        std::vector<std::string> args;
        for (int i = 0; argv[i]; ++i)
            args.emplace_back(argv[i]);
        return args;
    }

    void wait_for_input(const std::string& prompt = "Press anything... ")
    {
        std::cout << prompt << '\n';
        std::string input;
        std::getline(std::cin, input);
    }
}

#endif
