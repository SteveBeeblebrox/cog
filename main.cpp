
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <queue>
#include <regex>

#include "configstring/configstring.h"
#include "console.hpp"
#include "files.h"
#include "console.hpp"
#include "commands.h"

using namespace std;
using namespace console;
namespace fs = std::filesystem;

#define VERSION "1.0.0"

/// @brief Make a new project called NAME in a new folder called NAME. A default main.cpp and project.cfg is generated
void create_new_project(const string NAME) {
	printlnf("Creating project \"%s\"", NAME.c_str());
				
	files::mkdir(NAME);

	files::fwrite(NAME + "/.gitignore", R"""(build)""");

	files::fwrite(NAME + "/project.cfg", format(
R"""(# Project details;
project.name=%s;
project.version=1.0;
project.author=You!;

# Compilation Settings;
cpp.version=11;
cpp.warnings=all;
)""", NAME.c_str()));

	files::mkdir(NAME + "/src");
	files::fwrite(NAME + "/src/main.cpp",
R"""(#include<iostream>
using namespace std;

int main() {
	cout << "Project: " << PROJECT_NAME << " v" << PROJECT_VERSION << " by " << PROJECT_AUTHOR << endl;
	cout << "Hello World!" << endl;
}
)""");
}

/// @brief For the C++ source file at FILE
string get_make_dependencies(const fs::path FILE) {
	string line;
	string rule = FILE.stem().string() + ".o: " + FILE.string();
	queue<fs::path> dependencies;
	dependencies.push(FILE);

	// BFS for #includes branching out from FILE
	while (!dependencies.empty())  {  
		const fs::path NEXT = dependencies.front();
		dependencies.pop();

		ifstream file(NEXT);
		if(file.fail()) {
			eprintlnf("Could not access file %s", NEXT.string().c_str());
			continue;
		}

		// Search for #include "..." but ignore #include <...> since only user files should need to be compiled
		// Note that #includes in multiline comments may still be grabbed. 
		while (getline(file, line)) {
			regex pattern("^#include\\s+\"([^\"]+)\"$");
			smatch matches;
			string trimmed = configstring::stringlib::str_trim(line);
			if(regex_search(trimmed, matches, pattern)) {
				const string INCLUDE_TARGET = FILE.parent_path().string() + "/" + matches[1].str(); // match 0 is always the whole match
				rule += " " + INCLUDE_TARGET;
				dependencies.push(INCLUDE_TARGET);
			}
		}
		file.close();
	}
	return rule;
}

/// @brief Print cog's help message
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

/// @brief Load project config from a Project.config falling back to Project.cfg, project.config, and project.cfg in that order
configstring::ConfigObject get_config() {
	return configstring::parse(
		fs::exists("Project.config") ? files::fread("Project.config")
		: fs::exists("Project.cfg") ? files::fread("Project.cfg")
		: fs::exists("project.config") ? files::fread("project.config")
		: files::fread("project.cfg")
	);
}

// Excapes " -> \\" (One literal backslash and one quote)
string escape_quotes(const string ARG) {
	return configstring::stringlib::str_replace(ARG,"\"","\\\"");
}

/// @brief Build the project from the given config settings and set debug mode (defines the DEBUG macro for the project if true)
void build(const bool DEBUG, const configstring::ConfigObject CONFIG = get_config()) {
	files::mkdir("build");
	
	// version and author can be omitted while name is required
	string projectName, projectVersion = "1.0", projectAuthor = "anonymous";

	if(const auto VALUE = CONFIG.get("project.name")->as<configstring::String>()) {
		projectName = VALUE->getValue();
	} else {
		throw runtime_error("project.name is not a string");
	}

	if(CONFIG.has("project.version")) {
		if(const auto VALUE = CONFIG.get("project.version")->as<configstring::String>()) {
			projectVersion = VALUE->getValue();
		} else if(const auto value = CONFIG.get("project.version")->as<configstring::Number>()) {
			projectVersion = format("%.1f",value->getValue());
		} else {
			throw runtime_error("project.version is not a string or number");
		}
	}

	if(CONFIG.has("project.author")) {
		if(const auto VALUE = CONFIG.get("project.author")->as<configstring::String>()) {
			projectAuthor = VALUE->getValue();
		}  else {
			throw runtime_error("project.author is not a string");
		}
	}

	// All can be omitted
	int cppVersion = 11;
	string cppBin = "", cppWarnings = "";
	bool cppStrict = false;
	#pragma GCC warn move config reading into a pbr function, color output, have run use stderr to not clutter stdout, external deps??, --release uses make -B, os detection, pkg config lookup

	string dependencyRules = "", srcFiles = "";

	// Find all compilable c++ files
	for(const auto &entry : fs::recursive_directory_iterator("src")) {
		if(!fs::is_directory(entry)) {
			const auto PATH = entry.path();
			if(PATH.extension() == ".cpp") {
				srcFiles += PATH.string() + " ";
				dependencyRules += "build/" + get_make_dependencies(PATH) + '\n';
			}
		}
	}

	files::fwrite("build/makefile",
string("# autogenerated makefile\n")
+ "TARGET = build/" + projectName + "\n"
+ "SRC_FILES = " + configstring::stringlib::str_trim(srcFiles) + "\n"
+ "CXX = g++\n"
+ format("CFLAGS = -Wall -g -std=c++17 -DPROJECT_NAME=\"\\\"%s\\\"\" -DPROJECT_VERSION=\"\\\"%s\\\"\" -DPROJECT_AUTHOR=\"\\\"%s\\\"\"\n", escape_quotes(escape_quotes(projectName)).c_str(), escape_quotes(escape_quotes(projectVersion)).c_str(), escape_quotes(escape_quotes(projectAuthor)).c_str())
+ R"""(
OBJECTS = $(patsubst src/%.cpp,build/%.o,${SRC_FILES})

ifeq ($(shell echo "Windows"), "Windows")
	TARGET := $(TARGET).exe
	CFLAGS += -DWINDOWS
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(CXX) -o $@ $^

build/%.o: src/%.cpp
	@$(CXX) $(CFLAGS) -o $@ -c $<

# DEPENDENCIES
)""" + dependencyRules);

	const auto MAKE_RESULT = commands::run("make --makefile=build/makefile --silent");
	if(MAKE_RESULT != 0) {
		throw runtime_error("Error running make");
	}
}

/// @brief Build the project and then run it with args
void run(const bool DEBUG, const vector<string> ARGS, const configstring::ConfigObject CONFIG = get_config()) {
	build(DEBUG, CONFIG);

	string name;
	if(const auto VALUE = CONFIG.get("project.name")->as<configstring::String>()) {
		name = VALUE->getValue();
	} else {
		throw runtime_error("project.name is not a string");
	}

#ifdef WINDOWS
	name += ".exe";
#endif

	printlnf("Running project %s:", name.c_str());
	
	printlnf("Project exited with code %i", commands::run(("./build/" + name).c_str(), ARGS));
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
		} else if(ARG == "build") {
			vector<string> projectArgs;
			bool debug = true;
			for(int i = 2; i < argc; i++) {
				const auto ARG_I = string(argv[i]);
				if((ARG_I == "--release" || ARG_I == "-r")) {
					debug = false;
				} else {
					eprintlnf("Unexpected argument \"%s\"", ARG_I.c_str());
				}
			}
			build(debug);
		} else {
			eprintlnf("Unexpected argument \"%s\"", ARG.c_str());
		}
		return 0;
	} catch(const runtime_error &ERR) {
		eprintlnf("Runtime error \"%s\"", ERR.what());
	}
}
