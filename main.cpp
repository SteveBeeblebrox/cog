
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
#include "formatting.h"

#include "third_party/matchOS.h"

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
cpp.strict=false;
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

/// @brief For the C++ source file at FILE, recursively get all dependencies for make to watch timestamps
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
			eprintlnf("%sCould not access file %s%s", formatting::colors::fg::YELLOW, NEXT.string().c_str(), formatting::colors::fg::REVERT);
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

/// @brief If KEY exists, get KEY from CONFIG as a string or throw an error
void get_string_from_config(const configstring::ConfigObject &CONFIG, const string KEY, string &outValue) {
	if(const auto VALUE = CONFIG.get(KEY)->as<configstring::String>()) {
		outValue = VALUE->getValue();
	}  else {
		throw runtime_error(KEY + " is not a string");
	}
}

/// @brief Get KEY from CONFIG as a string or throw an error
void get_optional_string_from_config(const configstring::ConfigObject &CONFIG, const string KEY, string &outValue) {
	if(CONFIG.has(KEY)) {
		get_string_from_config(CONFIG, KEY, outValue);
	}
}

/// @brief Get KEY from CONFIG as a bool or throw an error
void get_bool_from_config(const configstring::ConfigObject &CONFIG, const string KEY, bool &outValue) {
	if(const auto VALUE = CONFIG.get(KEY)->as<configstring::Boolean>()) {
		outValue = VALUE->getValue();
	}  else {
		throw runtime_error(KEY + " is not a boolean");
	}
}

/// @brief If KEY exists, get KEY from CONFIG as a bool or throw an error
void get_optional_bool_from_config(const configstring::ConfigObject &CONFIG, const string KEY, bool &outValue) {
	if(CONFIG.has(KEY)) {
		get_bool_from_config(CONFIG, KEY, outValue);
	}
}

/// @brief Get KEY from CONFIG as a double or throw an error
void get_double_from_config(const configstring::ConfigObject &CONFIG, const string KEY, double &outValue) {
	if(const auto VALUE = CONFIG.get(KEY)->as<configstring::Number>()) {
		outValue = VALUE->getValue();
	}  else {
		throw runtime_error(KEY + " is not a number");
	}
}

/// @brief If KEY exists, get KEY from CONFIG as a double or throw an error
void get_optional_double_from_config(const configstring::ConfigObject &CONFIG, const string KEY, double &outValue) {
	if(CONFIG.has(KEY)) {
		get_double_from_config(CONFIG, KEY, outValue);
	}
}

/// @brief If KEY exists, get KEY from CONFIG as a version number
/// (Note that 1 or 1.0 would be interpreted as a number while 1.0.0 is a string, this grabs either)
void get_optional_version_from_config(const configstring::ConfigObject &CONFIG, const string KEY, string &outValue) {
	if(CONFIG.has(KEY)) {
		if(const auto VALUE = CONFIG.get(KEY)->as<configstring::String>()) {
			outValue = VALUE->getValue();
		} else if(const auto value = CONFIG.get(KEY)->as<configstring::Number>()) {
			outValue = format("%.1f",value->getValue());
		} else {
			throw runtime_error(KEY + " is not a string or number");
		}
	}
}

/// @brief Check that command COMMAND can be run by checking that COMMAND --version exists with code 0
void assert_command_exists(const string COMMAND, const string WHICH_KEY, const string ARG = "--version") {
	if(commands::run_and_read(COMMAND, vector<string> {ARG}).status != 0) {
		throw runtime_error(format("Command \"%s\" does not exist. Either set which.%s in project.cfg or install %s", COMMAND.c_str(), WHICH_KEY.c_str(), COMMAND.c_str()));
	}
}

struct Pkg {
	string name;
	string relation = "=";
	string version = "*";
};

/// @brief Build the project from the given config settings and set debug mode (defines the DEBUG macro for the project if true)
void build(const bool DEBUG, const configstring::ConfigObject CONFIG = get_config()) {
	files::mkdir("build");
	
	// version and author can be omitted while name is required
	string projectName, projectVersion = "1.0", projectAuthor = "anonymous";
	get_string_from_config(CONFIG, "project.name", projectName);
	get_optional_version_from_config(CONFIG, "project.version", projectVersion);
	get_optional_string_from_config(CONFIG, "project.author", projectAuthor);

	// All can be omitted
	string whichCPP = "g++", whichMake = "make", whichPkgConfig = "pkg-config";
	get_optional_string_from_config(CONFIG, "which.cpp", whichCPP);
	get_optional_string_from_config(CONFIG, "which.make", whichMake);
	get_optional_string_from_config(CONFIG, "which.pkg-config", whichPkgConfig);


	assert_command_exists(whichCPP, "cpp");
	assert_command_exists(whichMake, "make");

	bool cppStrict = false;
	get_optional_bool_from_config(CONFIG, "cpp.strict", cppStrict);

	double cppVersion = 11;
	get_optional_double_from_config(CONFIG, "cpp.version", cppVersion);

	// Look for third party dependencies in project.cfg; if found, require pkg-config command
	vector<Pkg> packages;
	regex pattern("^pkg.([a-zA-Z9-9_-]+)(<|>)$");
	smatch matches;
	for(const string KEY : CONFIG.keys()) {
		if(KEY.length() > 4 && KEY.rfind("pkg.",0) == 0) {
			Pkg pkg;
			if(regex_search(KEY, matches, pattern)) {
				pkg.name = matches[1].str(); // match 0 is always the whole match
				pkg.relation = matches[2].str() + "=";
			} else {
				pkg.name = KEY;
			}
			get_optional_version_from_config(CONFIG, KEY, pkg.version);
			packages.push_back(pkg);
		}
	}

	if(packages.size() > 0) {
		assert_command_exists(whichPkgConfig, "pkg-config");
	}

	#pragma GCC warn clean up cog info messages, pkg config lookup


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
+ "CXX = " + whichCPP + "\n"
+ format("CFLAGS = -std=c++%i -Wall%s -g -std=c++17 -DPROJECT_NAME=\"\\\"%s\\\"\" -DPROJECT_VERSION=\"\\\"%s\\\"\" -DPROJECT_AUTHOR=\"\\\"%s\\\"\"\n", (int)cppVersion, (cppStrict ? " -Werror -Wpedantic" : ""), escape_quotes(escape_quotes(projectName)).c_str(), escape_quotes(escape_quotes(projectVersion)).c_str(), escape_quotes(escape_quotes(projectAuthor)).c_str())
+ R"""(
OBJECTS = $(patsubst src/%.cpp,build/%.o,${SRC_FILES})

ifeq ($(shell echo "Windows"), "Windows")
	TARGET := $(TARGET).exe
endif
)"""
+ MAKE_MATCH_OS
+ R"""(
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(CXX) -o $@ $^

build/%.o: src/%.cpp
	@$(CXX) $(CFLAGS) -o $@ -c $<

# DEPENDENCIES
)""" + dependencyRules);
	

	vector<string> args = {"--makefile=build/makefile", "--silent"};
	if(!DEBUG) {
		args.push_back("--always-make");
	}

	const auto MAKE_RESULT = commands::run(whichMake, args);
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
	
	printlnf("Project exited with code %i", commands::run("./build/" + name, ARGS));
}

/// @brief Sends a warning to the console that ARG is not a valid argument
void warn_unexpected_argument(const char* ARG) {
	eprintlnf("%sUnexpected argument \"%s\"%s", formatting::colors::fg::YELLOW, ARG, formatting::colors::fg::REVERT);
}

/// @brief Sends a warning to the console that ARG is not a valid argument
void warn_unexpected_argument(const string ARG) {
	warn_unexpected_argument(ARG.c_str());
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
					warn_unexpected_argument(argv[i]);
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
					warn_unexpected_argument(ARG_I);
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
					warn_unexpected_argument(ARG_I);
				}
			}
			build(debug);
		} else {
			warn_unexpected_argument(ARG);
		}
		return 0;
	} catch(const runtime_error &ERR) {
		eprintlnf("%sRuntime error: %s%s", formatting::colors::fg::RED, ERR.what(), formatting::colors::fg::REVERT);
	}
}
