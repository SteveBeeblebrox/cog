
#include "ConfigObject.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include "values.hpp"
#include "../stringlib.h"

namespace configstring {
    /// @brief Returns the index of key in mKeys (and thus the index of the matching value in mValues) or -1 if it is not found
    long ConfigObject::getKeyIndex(const std::string key) const {
        const auto ITER = std::find(mKeys.begin(), mKeys.end(), key);
        if(ITER != mKeys.end())
            return ITER - mKeys.begin();
        else
            return -1;
    }

    /// @brief Used in operator= and copy constructor to duplicated pointed to values 
    void ConfigObject::deepCopyValues(const ConfigObject& OTHER) {
        for(Value* value : OTHER.mValues) {
            Value* copy;
            if (const auto N = value->as<Number>()) {
                copy = new Number(N->getValue());
            } else if (const auto B = value->as<Boolean>()) {
                copy = new Boolean(B->getValue());
            } else if (const auto S = value->as<String>()) {
                copy = new String(S->getValue());
            } else if (const auto num = value->as<Null>()) {
                copy = new Null;
            }
            mValues.push_back(copy);
        }
    }

    /// @brief Default constructor
    ConfigObject::ConfigObject() {}

    /// @brief The number of entries in this ConfigObject 
    size_t ConfigObject::size() const {
        return mKeys.size();
    }

    /// @brief A std::vector&lt;std::string&gt; of keys in this ConfigObject (Useful for iterating) 
    std::vector<std::string> ConfigObject::keys() const {
        return mKeys;
    }

    /// @brief True if key exists, false otherwise (Useful to validate a key before using get or remove)
    bool ConfigObject::has(const std::string key) const {
        return std::find(mKeys.begin(), mKeys.end(), key) != mKeys.end();
    }

    /// @brief Gets the value associated with key. Throws std::runtime_error if key does not exist
    Value* ConfigObject::get(const std::string key) const {
        const long INDEX = getKeyIndex(key);
        if(INDEX > -1)
            return mValues.at(INDEX);
        else 
            throw std::runtime_error("Unable to get nonexistant key \"" + key + "\".");
    }

    /// @brief Set the value associated with key. Adds the key and value if key does not exist
    void ConfigObject::set(const std::string key, Value* const P_value) {
        const long INDEX = getKeyIndex(key);
        if(INDEX > -1) {
            mValues.at(INDEX) = P_value;
        } else {
            mKeys.push_back(key);
            mValues.push_back(P_value);
        }
    }
    
    /// @brief Removes key and its associated value. Throws std::runtime_error if key does not exist. The removed value is returned and not deleted
    Value* ConfigObject::remove(const std::string key) {
        const long INDEX = getKeyIndex(key);
        if(INDEX > -1) {
            Value* const P_value =  mValues.at(INDEX);
            mKeys.erase(mKeys.begin() + INDEX);
            mValues.erase(mValues.begin() + INDEX);
            return P_value;
        } else { 
            throw std::runtime_error("Unable to remove nonexistant key \"" + key + "\".");
        }
    }

    /// @brief Get a representation of this ConfigObject in a writable form
    std::string ConfigObject::stringify() const {
        std::string result;
        for(std::string key : keys()) {
            const Value* value = get(key);
            result += stringlib::minimal_encode(key) + "=" + stringlib::minimal_encode(value->stringify()) + ";";
        }
        return result;
    }

    ConfigObject::ConfigObject(const ConfigObject& OTHER): mKeys(OTHER.mKeys) {
        deepCopyValues(OTHER);
    }

    ConfigObject::~ConfigObject() {
        for(const Value* value : mValues)
            delete value;
    }

    ConfigObject& ConfigObject::operator=(const ConfigObject& OTHER) {
        if(this != &OTHER) {
            this->~ConfigObject();
            mKeys = OTHER.mKeys;
            mValues = std::vector<Value*>();
            deepCopyValues(OTHER);
        }
        return *this;
    }
}