#include "confighelper.h"

#include <stdexcept>
#include <string>

#include "filesystem.h"
#include "configstring/configstring.h"
#include "files.h"
#include "console.hpp"
#include "commands.h"

using namespace std;

namespace fs = FILESYSTEM_NAMESPACE;

/// @brief If project.config exists, returns that otherwise returns project.cfg
std::string get_config_filename() {
	return files::fexists("project.config") ? "project.config" : "project.cfg";
}

/// @brief Load project config from project.config or and project.cfg in that order
configstring::ConfigObject get_config() {
	return configstring::parse(files::fread(get_config_filename()));
}

/// @brief If KEY exists, get KEY from CONFIG as a string or throw an error
void get_string_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, std::string &outValue) {
	if(const auto VALUE = CONFIG.get(KEY)->as<configstring::String>()) {
		outValue = VALUE->getValue();
	}  else {
		throw runtime_error(console::format("\"%s\" in project config is not a string", commands::escape_quotes(KEY).c_str()));
	}
}

/// @brief Get KEY from CONFIG as a string or throw an error
void get_optional_string_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, std::string &outValue) {
	if(CONFIG.has(KEY)) {
		get_string_from_config(CONFIG, KEY, outValue);
	}
}

/// @brief Get KEY from CONFIG as a bool or throw an error
void get_bool_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, bool &outValue) {
	if(const auto VALUE = CONFIG.get(KEY)->as<configstring::Boolean>()) {
		outValue = VALUE->getValue();
	}  else {
		throw runtime_error(console::format("\"%s\" in project config is not a boolean", commands::escape_quotes(KEY).c_str()));
	}
}

/// @brief If KEY exists, get KEY from CONFIG as a bool or throw an error
void get_optional_bool_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, bool &outValue) {
	if(CONFIG.has(KEY)) {
		get_bool_from_config(CONFIG, KEY, outValue);
	}
}

/// @brief Get KEY from CONFIG as a double or throw an error
void get_double_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, double &outValue) {
	if(const auto VALUE = CONFIG.get(KEY)->as<configstring::Number>()) {
		outValue = VALUE->getValue();
	}  else {
		throw runtime_error(console::format("\"%s\" in project config is not a number", commands::escape_quotes(KEY).c_str()));
	}
}

/// @brief If KEY exists, get KEY from CONFIG as a double or throw an error
void get_optional_double_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, double &outValue) {
	if(CONFIG.has(KEY)) {
		get_double_from_config(CONFIG, KEY, outValue);
	}
}

/// @brief If KEY exists, get KEY from CONFIG as a version number
/// (Note that 1 or 1.0 would be interpreted as a number while 1.0.0 is a string, this grabs either)
void get_optional_version_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, std::string &outValue) {
	if(CONFIG.has(KEY)) {
		if(const auto VALUE = CONFIG.get(KEY)->as<configstring::String>()) {
			outValue = VALUE->getValue();
		} else if(const auto value = CONFIG.get(KEY)->as<configstring::Number>()) {
			outValue = console::format("%.1f",value->getValue());
		} else {
			throw runtime_error(console::format("\"%s\" in project config is not a string or number", commands::escape_quotes(KEY).c_str()));
		}
	}
}
