#include "actions.h"

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
#include "confighelper.h"
#include "version.h"

#include "third_party/matchOS.h"

using namespace std;
using namespace console;

namespace colors = formatting::colors::fg;
namespace fmt = formatting;
namespace fs = std::filesystem;

/// @brief Make a new project called NAME in a new folder called NAME. A default main.cpp and project.cfg is generated
void create_new_project(const std::string NAME) {
	printlnf("%s%sCreating project \"%s\"%s%s", fmt::ITALIC, colors::CYAN, NAME.c_str(), colors::REVERT, fmt::REVERT_ITALIC);
				
	files::mkdir(NAME);

	files::fwrite(NAME + "/.gitignore", R"""(build)""");

	files::fwrite(NAME + "/project.cfg", format(
R"""(# Project Details;
project.name=%s;
project.version=1.0;
project.author=You!;

# Compilation Settings;
#cpp.version=11;
#cpp.strict=true;
#cpp.static=false;

#which.cpp=g++;
#which.make=make;
#which.pkg-config=pkg-config;

# Package Dependencies;
#pkg.libR=1.0;
#pkg.zlib>=2.0;
#pkg.libpcre2-8;
)""", NAME.c_str()));

	files::mkdir(NAME + "/src");
	files::fwrite(NAME + "/src/main.cpp",
R"""(#include <iostream>
using namespace std;

int main() {
	cout << "Project: " << PROJECT_NAME << " v" << PROJECT_VERSION << " by " << PROJECT_AUTHOR << endl;
	cout << "Hello World!" << endl;
}
)""");
}

/// @brief For the C++ source file at FILE, recursively get all dependencies for make to watch timestamps
std::string get_make_dependencies(const std::filesystem::path FILE) {
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
			eprintlnf("%sCould not access file %s%s", colors::YELLOW, NEXT.string().c_str(), colors::REVERT);
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
R"""(

           :@####@=           
     :#*. :+@.   @*:  +#-     
   :%%#:-%%%%*-:    .-+#%%=.*%%-   
  -@+                    =@=  
   .@#        ..        +@:   
...+%%.     +#*++*#+.     ##...       _/_/_/_/   _/_/_/_/   _/_/_/_/
@#++:     %%*      +@.    .++#@      _/         _/    _/   _/
@+       -@.       @+       -@     _/         _/    _/   _/  _/_/
@#==.     %%+      =@:     ==*@    _/         _/    _/   _/    _/
.::*%%     .+%%+==+#*.     *#:::   _/_/_/_/   _/_/_/_/   _/_/_/_/
   .@#       .::.       +@:   
  -@*                    =@=  
   -%%*.:##+:.     :=#%%-.+%%=   
     -##: :+@.   @*-..*%%=     
           :@#***@=           

	
=== cog v%s (C) 2023 Trin Wasinger ===

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

/// @brief Build the project from the given config settings and set debug mode (defines the DEBUG macro for the project if true)
void build(const bool DEBUG, const configstring::ConfigObject CONFIG) {
	files::mkdir("build");
	
	// Get project settings
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


	commands::assert_command_exists(whichCPP, "cpp");
	commands::assert_command_exists(whichMake, "make");

	bool cppStrict = false, cppStatic = false;
	get_optional_bool_from_config(CONFIG, "cpp.strict", cppStrict);
	get_optional_bool_from_config(CONFIG, "cpp.static", cppStatic);

	double cppVersion = 11;
	get_optional_double_from_config(CONFIG, "cpp.version", cppVersion);

	struct Pkg {
		string name;
		string relation = "=";
		string version = "*";
	};

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
				pkg.name = KEY.substr(4);
			}
			if(const auto VALUE = CONFIG.get(KEY)->as<configstring::Null>());
			else get_optional_version_from_config(CONFIG, KEY, pkg.version);
			packages.push_back(pkg);
		}
	}

	string pkgLinkFlags = "", pkgCompileFlags = "";
	if(packages.size() > 0) {
		commands::assert_command_exists(whichPkgConfig, "pkg-config");
	
		// Format and concat arguments for pkg-config, exec here and not in make to catch errors
		vector<string> pkgConfigCompileArgs;
		vector<string> pkgConfigLinkArgs;

		for(const Pkg PKG : packages) {
			if(PKG.version != "*") {
				pkgConfigCompileArgs.push_back(format("%s %s %s", PKG.name.c_str(), PKG.relation.c_str(), PKG.version.c_str()));
			} else {
				pkgConfigCompileArgs.push_back(format("%s", PKG.name.c_str()));
			}
		}

		// Copy
		pkgConfigLinkArgs = pkgConfigCompileArgs;

		// Customize
		pkgConfigCompileArgs.push_back("--cflags");
		pkgConfigLinkArgs.push_back("--libs");
		if(cppStatic) {
			pkgConfigLinkArgs.push_back("--static");
		}

		const auto PKG_CONFIG_COMPILE_RESULT = commands::run_and_read(whichPkgConfig, pkgConfigCompileArgs);
		if(PKG_CONFIG_COMPILE_RESULT.status != 0) {
			throw runtime_error("Error finding one or more packages");
		}

		const auto PKG_CONFIG_LINK_RESULT = commands::run_and_read(whichPkgConfig, pkgConfigLinkArgs);
		if(PKG_CONFIG_LINK_RESULT.status != 0) {
			throw runtime_error("Error finding one or more packages");
		}

		pkgCompileFlags = " " + PKG_CONFIG_COMPILE_RESULT.output;
		pkgLinkFlags = " " + PKG_CONFIG_LINK_RESULT.output;
	}

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
+ format("CFLAGS = -std=c++%i -Wall%s -g -std=c++17 -DPROJECT_NAME=\"\\\"%s\\\"\" -DPROJECT_VERSION=\"\\\"%s\\\"\" -DPROJECT_AUTHOR=\"\\\"%s\\\"\"", (int)cppVersion, (cppStrict ? " -Werror -Wpedantic" : ""), commands::escape_quotes(commands::escape_quotes(projectName)).c_str(), commands::escape_quotes(commands::escape_quotes(projectVersion)).c_str(), commands::escape_quotes(commands::escape_quotes(projectAuthor)).c_str()) + R"""(
OBJECTS = $(patsubst src/%.cpp,build/%.o,${SRC_FILES})

ifeq ($(shell echo "Windows"), "Windows")
	TARGET := $(TARGET).exe
endif
)""" + MAKE_MATCH_OS+ R"""(

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(CXX) -o $@ $^)""" + pkgLinkFlags + R"""(

build/%.o: src/%.cpp
	@$(CXX) $(CFLAGS) -o $@ -c $<)""" + pkgCompileFlags + R"""(

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
void run(const bool DEBUG, const std::vector<std::string> ARGS, const configstring::ConfigObject CONFIG) {
	build(DEBUG, CONFIG);

	string name;
	get_string_from_config(CONFIG, "project.name", name);

#ifdef WINDOWS
	name += ".exe";
#endif

	eprintlnf("%s%sRunning project %s:%s%s", fmt::ITALIC, colors::CYAN, name.c_str(), colors::REVERT, fmt::REVERT_ITALIC);
	
	eprintlnf("%s%sProject exited with code %i%s%s", fmt::ITALIC, colors::CYAN, commands::run("./build/" + name, ARGS), colors::REVERT, fmt::REVERT_ITALIC);
}