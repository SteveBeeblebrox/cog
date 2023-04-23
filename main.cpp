#include <string>
#include <vector>

#include "console.hpp"
#include "formatting.h"
#include "version.h"
#include "actions.h"

#include "third_party/matchOS.h"

using namespace std;
using namespace console;

namespace colors = formatting::colors::fg;
namespace fmt = formatting;
namespace fs = std::filesystem;

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
		eprintlnf("%sRuntime error: %s%s", colors::RED, ERR.what(), colors::REVERT);
	}
}
