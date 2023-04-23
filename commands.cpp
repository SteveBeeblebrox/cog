#include "commands.h"

#include <stdio.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <string>
#include "console.hpp"
#include "configstring/stringlib.h"

using namespace std;
using namespace console;

#ifdef WINDOWS
#define popen _popen
#define pclose _pclose
#endif
#include <iostream>
namespace commands {
    int run(const string COMMAND, const vector<string> ARGS) {
        string tail = "";
        for(const auto ARG : ARGS) {
            tail += format(" \"%s\"", configstring::stringlib::str_replace(ARG,"\"","\\\"").c_str());
        }
        return system(format("%s%s", COMMAND.c_str(), tail.c_str()).c_str());
    }
    CommandResult run_and_read(const string COMMAND, const vector<string> ARGS) {
        string tail = "";
        for(const auto ARG : ARGS) {
            tail += format(" \"%s\"", configstring::stringlib::str_replace(ARG,"\"","\\\"").c_str());
        }
        
        char buffer[128];
        string result = "";
        FILE* pipe = popen(format("%s%s", COMMAND.c_str(), tail.c_str()).c_str(), "r");
        if (!pipe) {
            throw runtime_error(format("Failed to run command \"%s\"", COMMAND.c_str()));
        }
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw runtime_error(format("Failed to run command \"%s\"", COMMAND.c_str()));
        }
        return CommandResult {result, pclose(pipe)};
    }
}