#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
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
#define getenv _getenv
#define putenv _putenv
#endif

namespace commands {
    /// @brief Run COMMAND with ARGS and return exit code (stdout is written to console)
    int run(const std::string COMMAND, const std::vector<std::string> ARGS) {
        string tail = "";
        for(const auto &ARG : ARGS) {
            tail += format(" \"%s\"", escape_quotes(ARG).c_str());
        }
        return system(format("%s%s", COMMAND.c_str(), tail.c_str()).c_str());
    }
    /// @brief Run COMMAND with ARGS and return exit code and command stdout (stdout is not written to console)
    CommandResult run_and_read(const std::string COMMAND, const std::vector<std::string> ARGS) {
        string tail = "";
        for(const auto &ARG : ARGS) {
            tail += format(" \"%s\"", escape_quotes(ARG).c_str());
        }
        
        char buffer[128];
        string result = "";
        FILE* pipe = popen(format("%s%s", COMMAND.c_str(), tail.c_str()).c_str(), "r");
        if (!pipe) {
            throw runtime_error(format("Failed to run command \"%s\"", escape_quotes(COMMAND).c_str()));
        }
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw runtime_error(format("Failed to run command \"%s\"", escape_quotes(COMMAND).c_str()));
        }
        return CommandResult {result, pclose(pipe)};
    }
    /// @brief Check that command COMMAND can be run by checking that COMMAND --version exists with code 0
    void assert_command_exists(const std::string COMMAND, const std::string WHICH_KEY, const std::string ARG) {
        if(commands::run_and_read(COMMAND, vector<string> {ARG}).status != 0) {
            throw runtime_error(format("Command \"%s\" does not exist. Either set which.%s in project.cfg or install %s", COMMAND.c_str(), WHICH_KEY.c_str(), COMMAND.c_str()));
        }
    }
    /// @brief Escapes " -> \\" (One literal backslash and one quote)
    std::string escape_quotes(const std::string ARG) {
        return configstring::stringlib::str_replace(ARG,"\"","\\\"");
    }

    /// @brief Escapes <space> -> \\<space> (One literal backslash and one space)
    std::string escape_spaces(const std::string ARG) {
        return configstring::stringlib::str_replace(ARG," ","\\ ");
    }

    /// @brief Gets the value of enviornment variable NAME 
    std::string get_env_var(const std::string NAME) {
        const char* VALUE = getenv(NAME.c_str());
        return VALUE == NULL ? std::string("") : std::string(VALUE);
    }

    /// @brief Sets the value of local enviornment variable NAME to VALUE
    bool set_env_var(const std::string NAME, const std::string VALUE) {
        return putenv(
#ifndef WINDOWS
            (char*)
#endif
            (NAME + "=" + VALUE).c_str()
        );
    }

    /// @brief Combines strings to form a path env variable separating entries with ':' (';' on Windows) 
    std::string concat_path(const std::string PATH) {
        return PATH;
    }
}