
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>


#include "configstring/configstring.h"
#include "console.hpp"

#pragma GCC warn dev
#include "files.h"
#include "console.hpp"
#include "commands.h"

using namespace std;
using namespace console;
namespace fs = std::filesystem;

#define VERSION "1.0.0"

void create_new_project(string name) {
    printlnf("Creating project \"%s\"", name.c_str());
                
    files::mkdir(name);

    files::fwrite(name + "/.gitignore", R"""(build)""");

    files::fwrite(name + "/project.cfg", format(
R"""(# Project details;
project.name=%s;
project.version=1.0;
project.author=You!;

# Compilation Settings;
cpp.version=11;
cpp.warnings=all;
)""", name.c_str()));

    files::mkdir(name + "/src");
    files::fwrite(name + "/src/main.cpp", format(
R"""(#include<iostream>
using namespace std;

int main() {
cout << "Project: " << PROJECT_NAME << " v" << PROJECT_VERSION << " by " << PROJECT_AUTHOR << endl;
cout << "Hello World!" << endl;
}
)""", name.c_str()));

    files::mkdir(name + "/build");
}

void show_help() {
    printlnf(
R"""(=== cog v%s (C) 2023 Trin Wasinger ===

Usage:
    cog [options...]

        Perform global operations
        Options:
            -h ? --help         Displays this message
            -v --version        Displays version info

    cog new [name]

        Creates a new project with a project.cfg and main.cpp file in a new directory.
        If name is given, that is used for the project and folder name

    cog run [cog options...] -- [project options...]

        Looks for a project.cfg file, builds the project, and runs it
        Cog Options:
            -r --release        Do not set the DEBUG macro
        Project Options:
            Any arguments placed after -- are sent to the target project instead of being interpreted by cog
)""", VERSION);
}

configstring::ConfigObject get_config() {
    return configstring::parse(
        fs::exists("Project.config") ? files::fread("Project.config")
        : fs::exists("project.cfg") ? files::fread("project.cfg")
        : fs::exists("project.config") ? files::fread("project.config")
        : files::fread("project.cfg")
    );
}

void build(bool debug, configstring::ConfigObject config = get_config()) {
    // version and author can be omitted while name is required
    string projectName, projectVersion = "1.0", projectAuthor = "anonymous";

// # Compilation Settings;
// cpp.version=11;
// cpp.warnings=all;

    if(const auto value = config.get("project.name")->as<configstring::String>()) {
        projectName = value->getValue();
    } else {
        throw runtime_error("project.name is not a string");
    }

    if(config.has("project.version")) {
        if(const auto value = config.get("project.version")->as<configstring::String>()) {
            projectVersion = value->getValue();
        } else if(const auto value = config.get("project.version")->as<configstring::Number>()) {
            projectVersion = to_string(value->getValue());
        } else {
            throw runtime_error("project.version is not a string or number");
        }
    }

    if(config.has("project.author")) {
        if(auto value = config.get("project.author")->as<configstring::String>()) {
            projectAuthor = value->getValue();
        }  else {
            throw runtime_error("project.author is not a string");
        }
    }

    files::fwrite("build/makefile",
R"""(

)""");

    const auto MAKE_RESULT = commands::run("make");
    if(MAKE_RESULT != 0) {
        throw runtime_error("Error running make");
    }
}

void run(bool debug, vector<string> args, const configstring::ConfigObject config = get_config()) {
    //build(debug, config);

    string name;
    if(const auto value = config.get("project.name")->as<configstring::String>()) {
        name = value->getValue();
    } else {
        throw runtime_error("project.name is not a string");
    }

#ifdef WINDOWS
    name += ".exe";
#endif

    printlnf("Running project %s:", name.c_str());
    
    printlnf("Project exited with code %i", commands::run(("./build/" + name).c_str(), args));
}

int main(int argc, char *argv[]) {
    try {
        if(argc < 2) {
            show_help();
            return 0;
        }
        
        const string ARG = string(argv[1]);
        if(ARG == "new") {
            string targetName = "UntitledProject";
            if(argc > 2) {
                targetName = argv[2];
                for(int i = 3; i < argc; i++) {
                    eprintlnf("Unexpected argument \"%s\"", argv[i]);
                }
            }
            
            create_new_project(targetName);
        } else if(ARG == "--help" || ARG == "-h" || ARG == "?") {
            show_help();
        } else if(ARG == "--version" || ARG == "-v") {
            printlnf("cog v%s", VERSION);
        } else if(ARG == "run") {
            vector<string> projectArgs;
            bool debug = true, readingThisArgs = true;
            for(int i = 2; i < argc; i++) {
                const auto ARG_I = string(argv[i]);
                if(readingThisArgs && ARG_I == "--") {
                    readingThisArgs = false;
                } else if(readingThisArgs && (ARG_I == "--release" || ARG_I == "-r")) {
                    debug = false;
                } else if(readingThisArgs) {
                    eprintlnf("Unexpected argument \"%s\"", ARG_I.c_str());
                } else {
                    projectArgs.push_back(ARG_I);
                }
            }
            run(debug, projectArgs);
        } else {
            eprintlnf("Unexpected argument \"%s\"", ARG.c_str());
        }
        return 0;
    } catch(runtime_error err) {
        eprintlnf("Runtime error \"%s\"", err.what());
    }
}
