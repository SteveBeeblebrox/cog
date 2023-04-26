#ifndef CONFIGOBJECT_H
#define CONFIGOBJECT_H

#include <string>
#include <vector>

#include "values.hpp"

namespace configstring {
    /// @brief A map of string keys and Values; all Values should be free store allocated and will automatically be deleted when the ConfigObject goes out of scope
    class ConfigObject final {
        private:
            /// @brief List of keys
            std::vector<std::string> mKeys;

            /// @brief List of values in order matching their keys
            std::vector<Value*> mValues;

            /// @brief Returns the index of key in mKeys (and thus the index of the matching value in mValues) or -1 if it is not found
            long getKeyIndex(const std::string key) const;

            /// @brief Used in operator= and copy constructor to duplicated pointed to values 
            void deepCopyValues(const ConfigObject& OTHER);
            
        public:
            /// @brief Default constructor
            ConfigObject();

            /// @brief The number of entries in this ConfigObject 
            size_t size() const;

            /// @brief A std::vector&lt;std::string&gt; of keys in this ConfigObject (Useful for iterating) 
            std::vector<std::string> keys() const;

            /// @brief True if key exists, false otherwise (Useful to validate a key before using get or remove)
            bool has(const std::string key) const;

            /// @brief Gets the value associated with key. Throws std::runtime_error if key does not exist
            Value* get(const std::string key) const;

            /// @brief Set the value associated with key. Adds the key and value if key does not exist
            void set(const std::string key, Value* const P_value);

            /// @brief Removes key and its associated value. Throws std::runtime_error if key does not exist. The removed value is returned and not deleted
            Value* remove(const std::string key);

            /// @brief Get a representation of this ConfigObject in a writable form
            std::string stringify() const;

            // Three shall be the number thou shalt count, and the number of the counting shall be three.
            ConfigObject(const ConfigObject &OTHER);
            ~ConfigObject();
            ConfigObject& operator=(const ConfigObject& OTHER);
    };
}
#endif