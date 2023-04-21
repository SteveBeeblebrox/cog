#ifndef STRINGLIB_H
#define STRINGLIB_H

#include<string>
#include <vector>

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
        std::string str_replace(std::string text, const std::string FROM, const std::string TO);

        /**
         * @brief Returns a vector of the input text splits on all of the delimiter. Note, if the string ends with the delimiter, no empty entry is added.
         * 
         * @param TEXT 
         * @param DELIMITER 
         * @return std::vector&lt;std::string&gt; 
         */
        std::vector<std::string> str_split(const std::string& TEXT, const char DELIMITER);

        /**
         * @brief Encodes "%" => "%25", "=" => "%3D", ";" => "%3B"
         * 
         * @param TEXT
         * @return std::string 
         */
        std::string minimal_encode(const std::string& TEXT);

        /**
         * @brief Encodes "%3B" => ";", "=" => "%3D", "%25" => "%"
         * 
         * @param TEXT
         * @return std::string 
         */
        std::string minimal_decode(const std::string& TEXT);

        /**
         * @brief Is the input empty ("") or only whitespace (\\n \\r \\t \\f \\v <space>)
         * 
         * @param TEXT 
         * @return bool
         */
        bool str_is_empty(const std::string& TEXT);

        /**
         * @brief Returns a copy of the input text in lowercase
         * 
         * @param text 
         * @return std::string 
         */
        std::string str_to_lower(std::string text);

        /**
         * @brief Returns a copy of the input with leading and trailing whitespace removed (\\n \\r \\t \\f \\v <space>)
         * 
         * @param text 
         * @return std::string 
         */
        std::string str_trim(std::string text);
    }
}
#endif