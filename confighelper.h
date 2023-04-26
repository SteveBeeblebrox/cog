#ifndef CONFIGHELPER_H
#define CONFIGHELPER_H
#include <stdexcept>
#include <string>

#include "configstring/configstring.h"

/// @brief If project.config exists, returns that otherwise returns project.cfg
std::string get_config_filename();

/// @brief Load project config from project.config or and project.cfg in that order
configstring::ConfigObject get_config() ;

/// @brief If KEY exists, get KEY from CONFIG as a string or throw an error
void get_string_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, std::string &outValue);

/// @brief Get KEY from CONFIG as a string or throw an error
void get_optional_string_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, std::string &outValue);

/// @brief Get KEY from CONFIG as a bool or throw an error
void get_bool_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, bool &outValue);

/// @brief If KEY exists, get KEY from CONFIG as a bool or throw an error
void get_optional_bool_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, bool &outValue);

/// @brief Get KEY from CONFIG as a double or throw an error
void get_double_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, double &outValue);

/// @brief If KEY exists, get KEY from CONFIG as a double or throw an error
void get_optional_double_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, double &outValue);

/// @brief If KEY exists, get KEY from CONFIG as a version number
/// (Note that 1 or 1.0 would be interpreted as a number while 1.0.0 is a string, this grabs either)
void get_optional_version_from_config(const configstring::ConfigObject &CONFIG, const std::string KEY, std::string &outValue);

#endif