#include <string>
#include "testing_files.h"

/// @brief Header with testing utils to include on projects
const std::string __TESTING__HPP =
R"""(#ifndef __TESTING_CPP__
#define __TESTING_CPP__
#ifndef TEST
#include <vector>
#include <string>
#include <exception>

namespace __Testing__ {
    enum Status {
        SKIPPED,PASSED,FAILED
    };

    typedef void(*Runner)();

    struct Position final {
        int line;
        std::string filename;
    };

    class Test final {
        private:
            const std::string name;
            Status status = Status::SKIPPED;
            std::string message = "";
        public:
            const Runner run;
            Test(const std::string &name, Runner run) : name(name), run(run) {}
            ~Test() {}
            void fail(std::string message = "Unexpected Error");
            void pass(std::string message = "Ok");
            std::string getName();
            std::string getMessage();
            std::string getStatus();
    };

    bool capture_output();

    bool release_output();

    std::string get_captured_output_for(Runner run);

    std::string get_captured_output();

    std::vector<Test>* get_tests();

    int register_test(Test test);

    class AssertionError final : public virtual std::exception {
        private:
            const std::string message;
            const Position position;
        public:
            AssertionError(const std::string& message, const Position position) : message(message), position(position) {}
            ~AssertionError() {}
            virtual const char* what() const noexcept override;

            Position get_position() const noexcept;
    };

    void assert(bool value, const char* message, Position position);

    void assert_output_equals(Runner run, const char* text, Position position);

    void assert_output_matches(Runner run, const char* pattern, Position position);
}

#define __CAT__(a,b) __CAT_INNER__(a,b)
#define __CAT_INNER__(a,b) a##b

#define __TEST__(NAME, BODY, ID) const int __CAT__(__CAT__(__register_test_,NAME),__) = __Testing__::register_test(__Testing__::Test(#NAME, []()->void {BODY;}))
#define TEST(NAME, BODY) __TEST__(NAME, BODY, __COUNTER__)

#define TASSERT(x) __Testing__::assert(x, #x, __Testing__::Position {__LINE__, __FILE__})
#define TPRINTS(BODY,TEXT) __Testing__::assert_output_equals([]()->void {BODY;}, TEXT, __Testing__::Position {__LINE__, __FILE__})
#define TPRINTMATCHES(BODY,PATTERN) __Testing__::assert_output_matches([]()->void {BODY;}, PATTERN, __Testing__::Position {__LINE__, __FILE__})

void __test__(int argc, char* argv[], char* env[]);
#endif
#endif)""";

/// @brief Header with testing utils to include on projects
const std::string __TESTING__CPP =
R"""(#include "__Testing__.hpp"

#include <vector>
#include <string>
#include <exception>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <regex>

namespace __Testing__ {
    void Test::fail(std::string message) {
        this->message = message;
        this->status = Status::FAILED;
    }
    void Test::pass(std::string message) {
        this->message = message;
        this->status = Status::PASSED;
    }
    std::string Test::getName() {
        return this->name;
    }
    std::string Test::getMessage() {
        return this->message;
    }
    std::string Test::getStatus() {
        switch(this->status) {
            case Status::SKIPPED: return "SKIPPED";
            case Status::PASSED: return "PASSED";
            case Status::FAILED: return "FAILED";
            default: return "???";
        }
    }

    namespace {
        std::streambuf* stdcout = nullptr;
        std::stringstream* get_capture_stream() {
            static std::stringstream* p = new std::stringstream();
            return p;
        }
    }

    bool capture_output() {
        if(!stdcout) {
            stdcout = std::cout.rdbuf(get_capture_stream()->rdbuf());
            return true;
        }
        return false;
    }

    bool release_output() {
        if(stdcout) {
            std::cout.rdbuf(stdcout);
            stdcout = nullptr;
            return true;
        }
        return false;
    }

    std::string get_captured_output_for(Runner run) {
        size_t i = get_capture_stream()->tellp();
        run();
        std::string output = get_capture_stream()->str().substr(i);
        return output;
    }

    std::string get_captured_output() {
        return get_capture_stream()->str();
    }

    std::vector<Test>* get_tests() {
        static std::vector<Test>* p = new std::vector<Test>();
        return p;
    }

    int register_test(Test test) {
        get_tests()->push_back(test);
        return get_tests()->size();
    }

    const char* AssertionError::what() const noexcept {
        return message.c_str();
    }

    Position AssertionError::get_position() const noexcept {
        return this->position;
    }

    void assert(bool value, const char* message, Position position) {
        if(!value) {
            throw AssertionError(std::string(message), position);
        }
    }

    void assert_output_equals(Runner run, const char* text, Position position) {
        std::string output = get_captured_output_for(run);
        assert(output == text, (std::string("Got output \"") + output + "\", but expected \"" + text + "\"").c_str(), position);
    }

    void assert_output_matches(Runner run, const char* pattern, Position position) {
        std::regex re(pattern);
        std::string output = get_captured_output_for(run);
        assert(std::regex_search(output, re), (std::string("Got output \"") + output + "\", but expected it to match /" + pattern + "/").c_str(), position);
    }

    void clean_up() {
        delete get_tests();
        delete get_capture_stream();
    }
}

void __test__(int argc, char* argv[], char* env[]) {
    std::ios_base::Init();
    std::cerr << "Testing..." << std::endl;
    std::vector<std::string> arguments(argv + 1, argv + argc);
    int passed = 0, skipped = 0;
    __Testing__::capture_output();
    for(__Testing__::Test &test : *__Testing__::get_tests()) {
        if(arguments.empty() || std::find(arguments.begin(), arguments.end(), test.getName()) != arguments.end()) {
            try {
                test.run();
                test.pass();
                passed++;
            } catch(__Testing__::AssertionError &e) {
                test.fail(std::string(e.what()) + " at " + e.get_position().filename + ":" + std::to_string(e.get_position().line));
            } catch(std::exception &e) {
                test.fail(e.what());
            } catch(std::string &e) {
                test.fail(e);
            } catch(...) {
                test.fail();
            }
        } else {
            skipped++;
        }
    }
    __Testing__::release_output();
    std::cerr << std::endl << "=== Test Results ===" << std::endl;
    for(__Testing__::Test &test : *__Testing__::get_tests()) {
        std::cerr << test.getName() << ": " << test.getStatus();
        if(!test.getMessage().empty()) {
            std::cerr << " (" << test.getMessage()  << ")";
        }
        std::cerr << std::endl;
    }
    std::cerr << "====================" << std::endl << std::endl;
    std::cerr << __Testing__::get_tests()->size() << " Total" << std::endl;
    std::cerr << skipped << " SKIPPED" << std::endl;
    std::cerr << passed << " PASSED" << std::endl;
    int failed = __Testing__::get_tests()->size() - skipped - passed;
    std::cerr << failed << " FAILED!" << std::endl;
    __Testing__::clean_up();
    exit(failed ? 1 : 0);
}

__attribute__((section(".init_array"))) void* __premain__ = (void*) &__test__;
)""";