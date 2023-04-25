#include "actions.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
#include <regex>
#include <map>

#include "filesystem.h"
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
namespace fs = FILESYSTEM_NAMESPACE;

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
std::string get_make_dependencies(const FILESYSTEM_NAMESPACE::path FILE) {
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
		/// @brief One of =, <=, >=
		string relation = "=";
		/// @brief version, * for any
		string version = "*";
		///@brief Features can make optional packages required
		bool required = true;
	};

	// Look for third party dependencies in project.cfg; if found, require pkg-config command
	vector<Pkg> packages;
	regex pkgPatternLG("^pkg\\??.([a-zA-Z0-9+_-]+)(<|>)$");
	regex pkgPatternSimple("^pkg\\??.([a-zA-Z0-9+_-]+)$");
	smatch matches;

	struct Feature {
		bool enabled = true;
		/// @brief Circular features allowed, used in BFS
		bool visited = false;
		/// @brief Features or packages this depends on
		vector<string> dependencies;
	};

	// Look for feature flags (done after reading all packages to ignore order)
	map<string, Feature*> features;
	regex featurePattern("^feature.([A-Z0-9_]+)(.required)?$");

	for(const string KEY : CONFIG.keys()) {
		const bool
			IS_PACKAGE = KEY.length() > 4 && KEY.rfind("pkg.",0) == 0,
			IS_OPTIONAL_PACKAGE = KEY.length() > 5 && KEY.rfind("pkg?.",0) == 0,
			IS_FEATURE = KEY.length() > 8 && KEY.rfind("feature.",0) == 0,
			IS_FEATURE_DTL = IS_FEATURE && KEY.length() > 8 + 9 && KEY.substr(KEY.length() - 9) == ".required",
			IS_FEATURE_NOTE = IS_FEATURE && KEY.length() > 8 + 5 && KEY.substr(KEY.length() - 5) == ".note";
		if(IS_PACKAGE || IS_OPTIONAL_PACKAGE) {
			Pkg pkg;
			if(regex_search(KEY, matches, pkgPatternLG)) {
				pkg.name = matches[1].str(); // match 0 is always the whole match
				pkg.relation = matches[2].str() + "=";
			} else if(regex_match(KEY, pkgPatternSimple)) {
				pkg.name = KEY.substr(4 + IS_OPTIONAL_PACKAGE);
			} else {
				throw runtime_error(format("Package \"%s\" in project config does not contain a valid package name", commands::escape_quotes(KEY).c_str()));
			}

			if(IS_OPTIONAL_PACKAGE) {
				pkg.required = false;
			}

			if(const auto VALUE = CONFIG.get(KEY)->as<configstring::Null>());
			else get_optional_version_from_config(CONFIG, KEY, pkg.version);

			packages.push_back(pkg);
		} else if(IS_FEATURE_DTL) {
			if(!regex_match(KEY, featurePattern)) {
				throw runtime_error(format("Feature detail \"%s\" in project config does not contain a valid feature name", commands::escape_quotes(KEY).c_str()));
			}

			const string FEATURE_NAME = KEY.substr(0,KEY.length()-9);
			Feature* pFeature;
			auto iter = features.find(FEATURE_NAME);
			if(iter != features.end()) {
				pFeature = iter->second;
			} else {
				pFeature = new Feature;
				features.insert({FEATURE_NAME, pFeature});
			}

			string value = "";
			get_string_from_config(CONFIG,KEY,value);
			for(const string ITEM : configstring::stringlib::str_split(value, (const char)',')) {
				pFeature->dependencies.push_back(configstring::stringlib::str_trim(ITEM));
			}
		} else if(IS_FEATURE && !IS_FEATURE_NOTE) {
			if(!regex_match(KEY, featurePattern)) {
				throw runtime_error(format("Feature \"%s\" in project config does not contain a valid feature name", commands::escape_quotes(KEY).c_str()));
			}

			Feature* pFeature;
			auto iter = features.find(KEY);
			if(iter != features.end()) {
				pFeature = iter->second;
			} else {
				pFeature = new Feature;
				features.insert({KEY, pFeature});
			}

			if(const auto VALUE = CONFIG.get(KEY)->as<configstring::Null>());
			else get_optional_bool_from_config(CONFIG, KEY, pFeature->enabled);
		}
	}

	queue<string> featuresToEnable;

	// Add default features
	for(auto const& [KEY, P_feature] : features) {
		if(P_feature->enabled) {
			featuresToEnable.push(KEY);
		}
	}

	// BFS to enable features and optional packages
	while(!featuresToEnable.empty()) {
		const string NEXT = featuresToEnable.front();
		auto iter = features.find(NEXT);
		Feature *pFeature;

		if(iter != features.end()) {
			pFeature = iter->second;
		} else {
			throw runtime_error(format("Cannot enable feature \"%s\" since it does not exist", commands::escape_quotes(NEXT).c_str()));
		}
		featuresToEnable.pop();

		if(pFeature->visited) {
			continue;
		} else {
			pFeature->visited = true;
		}

		pFeature->enabled = true;

		for(const string ITEM : pFeature->dependencies) {
			if(ITEM.length() > 4 && ITEM.rfind("pkg.",0) == 0) {
				auto iter = find_if(packages.begin(), packages.end(), [ITEM](const Pkg& PKG) { return PKG.name == ITEM.substr(4); });
				if(iter == packages.end()) {
					throw runtime_error(format("Cannot require package \"%s\" since is not specified", commands::escape_quotes(ITEM).c_str()));
				} else {
					iter->required = true;
				}
			} else if(ITEM.length() > 8 && ITEM.rfind("feature.",0) == 0) {
				featuresToEnable.push(ITEM);
			} else {
				throw runtime_error(format("Unexpected entry \"%s\" in \"%s.required\"", commands::escape_quotes(ITEM).c_str(), commands::escape_quotes(NEXT).c_str()));
			}
		}
	}

	string featureFlags = "";
	for(auto const& [KEY, P_feature] : features) {
		if(P_feature->enabled) {
			featureFlags += format(" -DFEATURE_%s", KEY.substr(8).c_str());
		}
		delete P_feature;
	}
	features.clear();

	packages.erase(
		remove_if(packages.begin(), packages.end(), [](const Pkg PKG) { return !PKG.required; }),
		packages.end()
	);

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
+ format("CFLAGS = -std=c++%i -Wall%s -g -std=c++17 -DPROJECT_NAME=\"\\\"%s\\\"\" -DPROJECT_VERSION=\"\\\"%s\\\"\" -DPROJECT_AUTHOR=\"\\\"%s\\\"\"", (int)cppVersion, (cppStrict ? " -Werror -Wpedantic" : ""), commands::escape_quotes(commands::escape_quotes(projectName)).c_str(), commands::escape_quotes(commands::escape_quotes(projectVersion)).c_str(), commands::escape_quotes(commands::escape_quotes(projectAuthor)).c_str()) + featureFlags + " " + R"""(
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
	const char CMD_PATH_SEPARATOR = '\\';
	name += ".exe";
#else
	const char CMD_PATH_SEPARATOR = '/';
#endif

	eprintlnf("%s%sRunning project %s:%s%s", fmt::ITALIC, colors::CYAN, name.c_str(), colors::REVERT, fmt::REVERT_ITALIC);
	
	eprintlnf("%s%sProject exited with code %i%s%s", fmt::ITALIC, colors::CYAN, commands::run(format(".%cbuild%c%s", CMD_PATH_SEPARATOR, CMD_PATH_SEPARATOR, name.c_str()), ARGS), colors::REVERT, fmt::REVERT_ITALIC);
}