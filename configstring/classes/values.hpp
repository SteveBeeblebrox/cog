#ifndef VALUES_HPP
#define VALUES_HPP

#include <string>

namespace configstring {
    class Value {
        public:
            /// @brief Destructor for Value abstract class, needed to suppress warning and incase derived class has destructor
            virtual ~Value();

            /// @brief Shorthand for dynamic_cast to T*
            template<typename T>
            T* as() {
                return dynamic_cast<T*>(this);
            }

            /// @brief Get the value of this Value in a writable form
            virtual std::string stringify() const = 0;
    };

    template<typename T>
    class ValueTemplateBase: public Value {
        protected:
            /// @brief The value stored
            T mValue;

            /// @brief Constructor to initialize value
            ValueTemplateBase(const T t) : mValue(t) {};
            
        public:
            /// @brief Get the value of this Value in a writable form
            virtual std::string stringify() const = 0;

            /// @brief Get the C++ typed value
            T getValue() const {
                return mValue;
            }

            /// @brief Set the C++ typed value
            void setValue(const T t) {
                mValue = t;
            }
    };

    // Note, Null has no value so it does not need to extend ValueTemplateBase
    class Null final: public Value {
        public:
            /// @brief Get the value of this Null in a writable form
            std::string stringify() const override;
    };

    class Number final: public ValueTemplateBase<double> {
        public:
            /// @brief Creates a number with value 0
            Number();

             /// @brief Creates a number from a double
            Number(const double D);

            /// @brief Get the value of this Number in a writable form
            std::string stringify() const override;
    };

    class Boolean final: public ValueTemplateBase<bool> {
        public:
             /// @brief Creates a boolean with value false
            Boolean();

            /// @brief Creates a boolean with an explicit value
            Boolean(const bool B);

            /// @brief Get the value of this Boolean in a writable form
            std::string stringify() const override;
    };

    class String final: public ValueTemplateBase<std::string> {
        public:
            /// @brief Creates an empty String
            String();

            /// @brief Creates a String from a constant character pointer
            String(const char* const C);

            /// @brief Creates a new String from a std::string
            String(const std::string S);

            /// @brief Get the value of this String in a writable form
            std::string stringify() const override;
    };
}
#endif