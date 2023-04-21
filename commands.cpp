#include "commands.h"

#include <stdio.h>
#include <string>
#include <stdexcept>
#include <vector>
#include "console.hpp"
#include "configstring/stringlib.h"

using namespace std;
using namespace console;

#ifdef WINDOWS
#define popen _popen
#define pclose _pclose
#endif

namespace commands {
    int run(const char* command, const vector<string> ARGS) {
        string tail = "";
        for(const auto ARG : ARGS) {
            tail += format(" \"%s\"", configstring::stringlib::str_replace(ARG,"\"","\\\"").c_str());
        }
        return system(format("%s%s", command, tail.c_str()).c_str());
    }
    CommandResult run_and_read(const char* command, const vector<string> ARGS) {
        string tail = "";
        for(const auto ARG : ARGS) {
            tail += format(" \"%s\"", configstring::stringlib::str_replace(ARG,"\"","\\\"").c_str());
        }
        
        char buffer[128];
        string result = "";
        FILE* pipe = popen(format("%s%s", command, tail).c_str(), "r");
        if (!pipe) {
            throw runtime_error(format("Failed to run command \"%s\"", command));
        }
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw runtime_error(format("Failed to run command \"%s\"", command));
        }
        return CommandResult {result, pclose(pipe)};
    }
}