#ifndef CONFIGSTRING_HPP
#define CONFIGSTRING_HPP

#include <string>
#include <vector>

#include "stringlib.h"
#include "classes/values.hpp"
#include "classes/ConfigObject.h"

namespace configstring {
    /// @brief Converts the ConfigObject to a string that can be parsed later
    std::string stringify(const ConfigObject& OBJECT);

    /// @brief Parses the value portion of an entry and returns a free store pointer to a wrapped C++ value
    Value* str_to_value(std::string text);

    /// @brief Parses a full source string into a ConfigObject with typed values
    ConfigObject parse(const std::string& TEXT);
}
#endif