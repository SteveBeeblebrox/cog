#include "stringlib.h"

#include <string>
#include <sstream>
#include <regex>

namespace configstring {
    namespace stringlib {
        /**
         * @brief Returns a copy of the input with all instances of from replaced with to
         * 
         * @param text 
         * @param FROM 
         * @param TO 
         * @return std::string 
         */
        std::string str_replace(std::string text, const std::string FROM, const std::string TO) {
            size_t start_pos = 0;
            while((start_pos = text.find(FROM, start_pos)) != std::string::npos) {
                text.replace(start_pos, FROM.length(), TO);
                start_pos += TO.length();
            }
            return text;
        }

        /**
         * @brief Returns a vector of the input text splits on all of the delimiter. Note, if the string ends with the delimiter, no empty entry is added.
         * 
         * @param TEXT 
         * @param DELIMITER 
         * @return std::vector&lt;std::string&gt; 
         */
        std::vector<std::string> str_split(const std::string& TEXT, const char DELIMITER) {
            std::vector<std::string> result;
            std::stringstream stream(TEXT);
            std::string tmp;

            while (getline(stream, tmp, DELIMITER))
                result.push_back(tmp);

            return result;
        }

        /**
         * @brief Encodes "%" => "%25", "=" => "%3D", ";" => "%3B"
         * 
         * @param TEXT
         * @return std::string 
         */
        std::string minimal_encode(const std::string& TEXT) {
            return str_replace(
                str_replace(
                    str_replace(
                        TEXT, "%", "%25"
                    ),
                    "=", "%3D"
                ),
                ";", "%3B"
            );
        }

        /**
         * @brief Encodes "%3B" => ";", "=" => "%3D", "%25" => "%"
         * 
         * @param TEXT
         * @return std::string 
         */
        std::string minimal_decode(const std::string& TEXT) {
            return str_replace(
                str_replace(
                    str_replace(
                        TEXT, "%3B", ";"
                    ),
                    "%3D", "="
                ),
                "%25", "%"
            );
        }

        /**
         * @brief Is the input empty ("") or only whitespace (\\n \\r \\t \\f \\v <space>)
         * 
         * @param TEXT 
         * @return bool
         */
        bool str_is_empty(const std::string& TEXT) {
            if(TEXT == "")
                return true;
            else
                return !std::regex_search(TEXT, std::regex(R"(\S)", std::regex_constants::ECMAScript));
        }

        /**
         * @brief Returns a copy of the input text in lowercase
         * 
         * @param text 
         * @return std::string 
         */
        std::string str_to_lower(std::string text) {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){ return std::tolower(c); });
            return text;
        }

        /**
         * @brief Returns a copy of the input with leading and trailing whitespace removed (\\n \\r \\t \\f \\v <space>)
         * 
         * @param text 
         * @return std::string 
         */
        std::string str_trim(std::string text) {
            return std::regex_replace(std::regex_replace(text, std::regex(R"(^\s*)", std::regex_constants::ECMAScript), ""), std::regex(R"(\s*$)", std::regex_constants::ECMAScript), "");
        }
    }
}