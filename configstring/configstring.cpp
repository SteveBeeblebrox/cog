#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <algorithm>
#include <exception>

#include "configstring.h"

using namespace configstring::stringlib;

namespace configstring {
    /// @brief Converts the ConfigObject to a string that can be parsed later
    std::string stringify(const ConfigObject& OBJECT) {
        return OBJECT.stringify();
    }

    /// @brief Parses the value portion of an entry and returns a free store pointer to a wrapped C++ value
    Value* str_to_value(std::string text) {
        const std::string LOWER = str_to_lower(text);
        if(LOWER == "true") {
            return new Boolean(true);
        } else if(LOWER == "false") {
            return new Boolean(false);
        } else if(LOWER == "null" || str_is_empty(text)) {
            return new Null;
        } else if(std::regex_match(text, std::regex(R"(^\d+(?:\.\d+)?$)", std::regex_constants::ECMAScript))) {
            return new Number(std::stod(text));
        } else {
            return new String(std::regex_replace(text, std::regex("^\"([" R"(\s\S)" "]*)\"$", std::regex_constants::ECMAScript), "$1"));
        }
    }

    /// @brief Parses a full source string into a ConfigObject with typed values
    ConfigObject parse(const std::string& TEXT) {
        ConfigObject result;
        for(const std::string ENTRY : str_split(TEXT, (const char)';')) {
            if(!str_is_empty(ENTRY)) {
                if(str_trim(ENTRY).rfind("#",0) == 0)
                    continue;
                const std::vector<std::string> KEYVALUE = str_split(ENTRY, (const char) '=');
                if(KEYVALUE.size() == 1) {
                    result.set(minimal_decode(str_trim(KEYVALUE.at(0))), str_to_value(""));
                }
                else if(KEYVALUE.size() == 2) {
                    result.set(minimal_decode(str_trim(KEYVALUE.at(0))), str_to_value(minimal_decode(str_trim(KEYVALUE.at(1)))));
                } else {
                    throw std::runtime_error("Not all entries are in the format \"key=value\".");
                }
            }
        }
        return result;
    }
}