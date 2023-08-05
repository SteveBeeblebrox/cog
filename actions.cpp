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
#include "testing_files.h"

using namespace std;
using namespace console;

namespace colors = formatting::colors::fg;
namespace fmt = formatting;
namespace fs = FILESYSTEM_NAMESPACE;

/// @brief Gets the directory for different build types (NORMAL => "build", TEST => "test")
std::string get_build_dir(const BuildType TYPE) {
	switch(TYPE) {
		case BuildType::TEST: return "test";
		default: return "build";
	}
}

/// @brief Make a new project called NAME in a new folder called NAME. A default main.cpp and project.cfg is generated
void create_new_project(const std::string NAME) {
	files::validate_fname(NAME);

	printlnf("%s%sCreating project \"%s\"%s%s", fmt::ITALIC, colors::CYAN, NAME.c_str(), colors::REVERT, fmt::REVERT_ITALIC);
				
	files::mkdir(NAME);

	files::fwrite(NAME + "/.gitignore",
R"""(build
test
)"""
	);

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
#pkg.prettyprint;

# Feature Flags;
#feature.LOGGING=true;
#feature.ANSI_COLORS=false;
#feature.ANSI_COLORS.notes=Enables Console Formatting;
#feature.ANSI_COLORS.required=feature.LOGGING,pkg.prettyprint;
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

		ifstream file(NEXT.string());
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
			-h ? --help         		Displays this message
			-v --version        		Displays version info

	cog new [name]

		Creates a new project with a project.cfg and main.cpp file in a new directory.
		If name is given, that is used for the project and folder name

	cog features
	
		Show a table of features supported by the current project

	cog test [cog options...] -- [tests...]

		Runs the given tests for project.cfg or all tests if none are specified.
		Cake, and grief counseling, will be available at the conclusion of the test
		Cog Options:
			-x --no-default-features	Disable any default project features
			-F <V> --feature <V>		Enables project feature V and any dependencies
		Tests:
			If any test names are listed after the --, only those are run

	cog build [cog options...]
	cog run [cog options...] -- [project options...]

		Looks for a project.cfg file, builds the project, and runs it
		Cog Options:
			-r --release        		Do not set the DEBUG macro and force rebuild
			-x --no-default-features	Disable any default project features
			-F <V> --feature <V>		Enables project feature V and any dependencies
		Project Options:
			Any arguments placed after -- are sent to the target project instead of being interpreted by cog
)""", VERSION);
}

/// @brief Build the project from the given config settings and set debug mode (defines the DEBUG macro for the project if true)
void build(const bool DEBUG, const bool DEFAULT_FEATURES, const std::vector<std::string> FEATURES, const BuildType TYPE, const configstring::ConfigObject CONFIG) {
	std::string BUILD_DIR = get_build_dir(TYPE);

	files::mkdir(BUILD_DIR);

	if(TYPE == BuildType::TEST) {
		files::fwrite(BUILD_DIR + "/__Testing__.hpp", __TESTING__HPP);
		files::fwrite(BUILD_DIR + "/__Testing__.cpp", __TESTING__CPP);
	}

	// Get project settings
	// version and author can be omitted while name is required
	string projectName, projectVersion = "1.0", projectAuthor = "anonymous";
	get_string_from_config(CONFIG, "project.name", projectName);
	files::validate_fname(projectName);
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
		/// @brief Version, * for any
		string version = "*";
		///@brief Features can make optional packages required
		bool required = true;
	};

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

	map<string, Feature*> features;
	string featureFlags = "";

	// Read in features and third party packages from config
	try {
		const string PKG_PREFIX = "pkg.", OPTIONAL_PKG_PREFIX = "pkg?.", FEATURE_PREFIX = "feature.", FEATURE_NOTE_SUFFIX = ".notes", FEATURE_REQUIRED_SUFFIX = ".required";
		regex featurePattern(format("^%s([A-Z0-9_]+)(%s)?$", FEATURE_PREFIX.c_str(), FEATURE_REQUIRED_SUFFIX.c_str()));
		for(const string &KEY : CONFIG.keys()) {
			const bool
				IS_PACKAGE = KEY.length() > PKG_PREFIX.length() && KEY.rfind(PKG_PREFIX,0) == 0,
				IS_OPTIONAL_PACKAGE = KEY.length() > OPTIONAL_PKG_PREFIX.length() && KEY.rfind(OPTIONAL_PKG_PREFIX,0) == 0,
				IS_FEATURE = KEY.length() > FEATURE_PREFIX.length() && KEY.rfind(FEATURE_PREFIX,0) == 0,
				IS_FEATURE_DTL = IS_FEATURE && KEY.length() > FEATURE_PREFIX.length() + FEATURE_REQUIRED_SUFFIX.length() && KEY.substr(KEY.length() - FEATURE_REQUIRED_SUFFIX.length()) == FEATURE_REQUIRED_SUFFIX,
				IS_FEATURE_NOTE = IS_FEATURE && KEY.length() > FEATURE_PREFIX.length() + FEATURE_NOTE_SUFFIX.length() && KEY.substr(KEY.length() - FEATURE_NOTE_SUFFIX.length()) == FEATURE_NOTE_SUFFIX;
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
				for(const string &ITEM : configstring::stringlib::str_split(value, (const char)',')) {
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

				pFeature->enabled = pFeature->enabled && DEFAULT_FEATURES;
			}
		}

		queue<string> featuresToEnable;

		for(const string &FEATURE : FEATURES) {
			featuresToEnable.push("feature." + FEATURE);
		}

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

			for(const string &ITEM : pFeature->dependencies) {
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

		for(auto const& [KEY, P_feature] : features) {
			if(P_feature->enabled) {
				featureFlags += format(" -DFEATURE_%s", KEY.substr(8).c_str());
			}
			delete P_feature;
		}
		features.clear();
	} catch(const runtime_error &ERR) {
		for(auto const& [KEY, P_feature] : features) {
			delete P_feature;
		}
		features.clear();
		throw ERR;
	}

	bool forceRebuild = false || !DEBUG;

	// Lock management to check if forced rebuild is needed
	{
		const string LOCK_FILE = BUILD_DIR + "/project.lock";
		const string PROJECT_LOCK_KEY = "project.identity";
		const string FEATURE_LOCK_KEY = "features.identity";
		const string RELEASE_LOCK_KEY = "release.identity";
		
		configstring::ConfigObject lockConfig;
		if(files::fexists(LOCK_FILE)) {
			lockConfig = configstring::parse(files::fread(LOCK_FILE));
		}

		string currentProjectHash = format("%x",hash<string>{}(files::fread(get_config_filename())));
		string oldProjectFileHash = "";
		get_optional_string_from_config(lockConfig,PROJECT_LOCK_KEY,oldProjectFileHash);

		string currentFeatureHash = format("%x",hash<string>{}(featureFlags));
		string oldFeatureHash = "";
		get_optional_string_from_config(lockConfig,FEATURE_LOCK_KEY,oldFeatureHash);
		
		bool isRelease = !DEBUG;
		bool wasRelease = false;
		get_optional_bool_from_config(lockConfig,RELEASE_LOCK_KEY,wasRelease);

		// If project.cfg changes, force a rebuild and update lock
		if(oldProjectFileHash != currentProjectHash) {
			forceRebuild = true;
			lockConfig.set(PROJECT_LOCK_KEY, new configstring::String(currentProjectHash));
		}

		// If features change, force a rebuild and update lock
		if(oldFeatureHash != currentFeatureHash) {
			forceRebuild = true;
			lockConfig.set(FEATURE_LOCK_KEY, new configstring::String(currentFeatureHash));
		}

		// If build mode change, force a rebuild and update lock
		if(wasRelease != isRelease) {
			forceRebuild = true;
			lockConfig.set(RELEASE_LOCK_KEY, new configstring::Boolean(isRelease));
		}

		// Update lock file
		files::fwrite(LOCK_FILE, lockConfig.stringify());
		
		// lockConfig will automatically delete its values when it goes out of scope
	}


	// Remove unneeded packages
	packages.erase(
		remove_if(packages.begin(), packages.end(), [](const Pkg PKG) { return !PKG.required; }),
		packages.end()
	);

	// Get required package info
	string pkgLinkFlags = "", pkgCompileFlags = "";
	if(packages.size() > 0) {
		commands::assert_command_exists(whichPkgConfig, "pkg-config");
	
		// Format and concat arguments for pkg-config, exec here and not in make to catch errors
		vector<string> pkgConfigCompileArgs;
		vector<string> pkgConfigLinkArgs;

		for(const Pkg &PKG : packages) {
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

		const string PKG_CONFIG_PATH = "PKG_CONFIG_PATH";
		commands::set_env_var(PKG_CONFIG_PATH, commands::concat_path(commands::get_env_var(PKG_CONFIG_PATH), "packages"));

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
	// Find all compilable c++ files and generate dependencies
	for(const auto &entry : fs::recursive_directory_iterator("src")) {
		if(!fs::is_directory(entry)) {
			const auto PATH = entry.path();
			if(PATH.extension() == ".cpp") {
				srcFiles += PATH.string() + " ";
				dependencyRules += BUILD_DIR + "/" + get_make_dependencies(PATH) + '\n';
			}
		}
	}

	if(TYPE == BuildType::TEST) {
		srcFiles += std::string(" ") + BUILD_DIR + "/__Testing__.cpp";
	}

	files::fwrite(BUILD_DIR + "/makefile",
string("# autogenerated makefile\n")
+ "TARGET = \"" + BUILD_DIR + "/" + commands::escape_spaces(commands::escape_quotes(projectName)) + "\"\n"
+ "SRC_FILES = " + configstring::stringlib::str_replace(configstring::stringlib::str_trim(srcFiles), "\\", "/") + "\n"
+ "CXX = " + whichCPP + "\n"
+ format("CFLAGS = -std=c++%i -Wall%s -g -std=c++17 -DPROJECT_NAME=\"\\\"%s\\\"\" -DPROJECT_VERSION=\"\\\"%s\\\"\" -DPROJECT_AUTHOR=\"\\\"%s\\\"\"", (int)cppVersion, (cppStrict ? " -Werror -Wpedantic" : ""), commands::escape_quotes(commands::escape_quotes(projectName)).c_str(), commands::escape_quotes(commands::escape_quotes(projectVersion)).c_str(), commands::escape_quotes(commands::escape_quotes(projectAuthor)).c_str()) + featureFlags + " " + R"""(
OBJECTS = $(patsubst src/%.cpp,)""" + BUILD_DIR + R"""(/%.o,${SRC_FILES})

ifeq ($(shell echo "Windows"), "Windows")
	TARGET := $(TARGET).exe
endif
)""" + MAKE_MATCH_OS + R"""(

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(CXX) -o $@ $^)""" + (cppStatic ? " -static" : "") + pkgLinkFlags + R"""(

)""" + BUILD_DIR + R"""(/%.o: src/%.cpp
	@$(CXX) $(CFLAGS) -o $@ -c $<)""" + pkgCompileFlags + std::string(TYPE == BuildType::TEST ? std::string(" -include ") + BUILD_DIR + "/__Testing__.hpp" : " -D'TEST(...)='") + R"""(

)""" + dependencyRules);

	vector<string> args = {"--makefile=" + BUILD_DIR + "/makefile", "--silent"};
	if(forceRebuild) {
		args.push_back("--always-make");
	}

	const auto MAKE_RESULT = commands::run(whichMake, args);
	if(MAKE_RESULT != 0) {
		throw runtime_error("Error running make");
	}
}

/// @brief Build the project and then run it with args
void run(const bool DEBUG, const bool DEFAULT_FEATURES, const std::vector<std::string> FEATURES, const std::vector<std::string> ARGS, const BuildType TYPE, const configstring::ConfigObject CONFIG) {
	build(DEBUG, DEFAULT_FEATURES, FEATURES, TYPE, CONFIG);

	string name;
	get_string_from_config(CONFIG, "project.name", name);

#ifdef WINDOWS
	const char CMD_PATH_SEPARATOR = '\\';
	name += ".exe";
#else
	const char CMD_PATH_SEPARATOR = '/';
#endif

	eprintlnf("%s%s%s %s:%s%s", fmt::ITALIC, colors::CYAN, (TYPE == BuildType::TEST ? "Testing" : "Running project"), name.c_str(), colors::REVERT, fmt::REVERT_ITALIC);
	
	eprintlnf("%s%sProject exited with code %i%s%s", fmt::ITALIC, colors::CYAN, commands::run(format(".%c%s%c\"%s\"", CMD_PATH_SEPARATOR, get_build_dir(TYPE).c_str(), CMD_PATH_SEPARATOR, commands::escape_quotes(name).c_str()), ARGS), colors::REVERT, fmt::REVERT_ITALIC);
}