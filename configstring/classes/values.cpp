#include "values.hpp"

#include <string>
#include <sstream>

namespace configstring {
    //================== class Value ===================//
    /// @brief Destructor for Value abstract class, needed to suppress warning and incase derived class has destructor
    Value::~Value() {}

    //================== class Null ===================//
    /// @brief Get the value of this Null in a writable form
    std::string Null::stringify() const {
        return "null";
    }

    //================== class Number ===================//
    /// @brief Creates a number with value 0
    Number::Number(): ValueTemplateBase(0) {}

    /// @brief Creates a number from a double
    Number::Number(const double D): ValueTemplateBase(D) {}

    /// @brief Get the value of this Number in a writable form
    std::string Number::stringify() const {
        std::stringstream stream;
        stream << mValue;
        return stream.str();
    }

    //================== class Boolean ===================//
    /// @brief Creates a boolean with value false
    Boolean::Boolean(): ValueTemplateBase(false) {}

    /// @brief Creates a boolean with an explicit value
    Boolean::Boolean(const bool B): ValueTemplateBase(B) {}

    /// @brief Get the value of this Boolean in a writable form
    std::string Boolean::stringify() const {
        return mValue ? "true" : "false";
    }

    //================== class String ===================//
    /// @brief Creates an empty String
    String::String(): ValueTemplateBase("") {}
    
    /// @brief Creates a String from a constant character pointer
    String::String(const char* const C): ValueTemplateBase(std::string(C)) {}
    
    /// @brief Creates a new String from a std::string
    String::String(const std::string S): ValueTemplateBase(S) {}

    /// @brief Get the value of this String in a writable form
    std::string String::stringify() const {
        return "\"" + mValue  + "\"";
    }
}