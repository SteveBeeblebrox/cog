#include <string>
#include <vector>
#include <regex>

#include "console.hpp"
#include "formatting.h"
#include "version.h"
#include "actions.h"
#include "files.h"

#include "third_party/matchOS.h"

using namespace std;
using namespace console;

namespace colors = formatting::colors::fg;
namespace fmt = formatting;

/// @brief Sends a warning to the console that ARG is not a valid argument
void warn_unexpected_argument(const char* ARG) {
	eprintlnf("%sUnexpected argument \"%s\"%s", colors::YELLOW, ARG, colors::REVERT);
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
			vector<string> projectArgs, features;
			bool debug = true, readingThisArgs = true, defaultFeatures = true;
			for(int i = 2; i < argc; i++) {
				const auto ARG_I = string(argv[i]);
				if(readingThisArgs && ARG_I == "--") {
					readingThisArgs = false;
				} else if(readingThisArgs && (ARG_I == "--release" || ARG_I == "-r")) {
					debug = false;
				} else if(readingThisArgs && (ARG_I == "--no-default-features" || ARG_I == "-x")) {
					defaultFeatures = false;
				} else if(readingThisArgs && (ARG_I == "--feature" || ARG_I == "-F")) {
					if(i + 1 < argc) {
						features.push_back(argv[++i]);
					} else {
						warn_unexpected_argument(ARG_I);
					}
				} else if(readingThisArgs) {
					warn_unexpected_argument(ARG_I);
				} else {
					projectArgs.push_back(ARG_I);
				}
			}
			run(debug, defaultFeatures, features, projectArgs);
		} else if(ARG == "build") {
			vector<string> features;
			bool debug = true, defaultFeatures = true;
			for(int i = 2; i < argc; i++) {
				const auto ARG_I = string(argv[i]);
				if((ARG_I == "--release" || ARG_I == "-r")) {
					debug = false;
				} else if((ARG_I == "--no-default-features" || ARG_I == "-x")) {
					defaultFeatures = false;
				} else if((ARG_I == "--feature" || ARG_I == "-F")) {
					if(i + 1 < argc) {
						features.push_back(argv[++i]);
					} else {
						warn_unexpected_argument(ARG_I);
					}
				} else {
					warn_unexpected_argument(ARG_I);
				}
			}
			build(debug, defaultFeatures, features);
		} else if(ARG == "features") {
			const configstring::ConfigObject CONFIG = get_config();

			string projectName;
			get_string_from_config(CONFIG, "project.name", projectName);
			files::validate_fname(projectName);

			printlnf("Features declared by %s:", projectName.c_str());

			bool hasAny = false;
			const string FEATURE_PREFIX = "feature.", FEATURE_NOTE_SUFFIX = ".notes", FEATURE_REQUIRED_SUFFIX = ".required";
			regex featurePattern(format("^%s([A-Z0-9_]+)$", FEATURE_PREFIX.c_str()));
			for(const string KEY : CONFIG.keys()) {
				const bool
					IS_FEATURE = KEY.length() > FEATURE_PREFIX.length() && KEY.rfind(FEATURE_PREFIX,0) == 0,
					IS_FEATURE_DTL = IS_FEATURE && KEY.length() > FEATURE_PREFIX.length() + FEATURE_REQUIRED_SUFFIX.length() && KEY.substr(KEY.length() - FEATURE_REQUIRED_SUFFIX.length()) == FEATURE_REQUIRED_SUFFIX,
					IS_FEATURE_NOTE = IS_FEATURE && KEY.length() > FEATURE_PREFIX.length() + FEATURE_NOTE_SUFFIX.length() && KEY.substr(KEY.length() - FEATURE_NOTE_SUFFIX.length()) == FEATURE_NOTE_SUFFIX;


				if(IS_FEATURE && !IS_FEATURE_DTL && !IS_FEATURE_NOTE && regex_match(KEY, featurePattern)) {
					hasAny = true;
					
					string notes = "";
					if(CONFIG.has(KEY + FEATURE_NOTE_SUFFIX)) {
						if(const auto VALUE = CONFIG.get(KEY + FEATURE_NOTE_SUFFIX)->as<configstring::Null>());
						else get_optional_string_from_config(CONFIG , KEY + FEATURE_NOTE_SUFFIX, notes);
					}

					bool defaultValue = true;
					if(const auto VALUE = CONFIG.get(KEY)->as<configstring::Null>());
					else get_optional_bool_from_config(CONFIG , KEY, defaultValue);

					printlnf("\t%s%s%s", KEY.substr(8).c_str(), (notes != "" ? " - " + notes : "").c_str(), (defaultValue ? " (default)" : ""));
				}
			}

			if(!hasAny) {
				printlnf("\t(None)", "");
			}
		} else {
			warn_unexpected_argument(ARG);
		}
		return 0;
	} catch(const runtime_error &ERR) {
		eprintlnf("%sRuntime error: %s%s", colors::RED, ERR.what(), colors::REVERT);
	}
}
