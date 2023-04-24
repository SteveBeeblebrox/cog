#ifndef ACTIONS_H
#define ACTIONS_H

#include <string>
#include <vector>

#include "filesystem.h"
#include "confighelper.h"
#include "configstring/configstring.h"

/// @brief Make a new project called NAME in a new folder called NAME. A default main.cpp and project.cfg is generated
void create_new_project(const std::string NAME);

/// @brief For the C++ source file at FILE, recursively get all dependencies for make to watch timestamps
std::string get_make_dependencies(const FILESYSTEM_NAMESPACE::path FILE);

/// @brief Print cog's help message
void show_help();

/// @brief Build the project from the given config settings and set debug mode (defines the DEBUG macro for the project if true)
void build(const bool DEBUG, const configstring::ConfigObject CONFIG = get_config());

/// @brief Build the project and then run it with args
void run(const bool DEBUG, const std::vector<std::string> ARGS, const configstring::ConfigObject CONFIG = get_config());

#endif