//=================//
#define COMPILE_TESTS
#define TEST_MAIN main
//__test_main__
#define MAIN _main
// main
//=================//

#pragma once

#ifdef COMPILE_TESTS

#include <vector>
#include <string>
#include <exception>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <regex>

namespace __TestRegistry__ {
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
            void fail(std::string message = "Unexpected Error") {
                this->message = message;
                this->status = Status::FAILED;
            }
            void pass(std::string message = "Ok") {
                this->message = message;
                this->status = Status::PASSED;
            }
            std::string getName() {
                return this->name;
            }
            std::string getMessage() {
                return this->message;
            }
            std::string getStatus() {
                switch(this->status) {
                    case Status::SKIPPED: return "SKIPPED";
                    case Status::PASSED: return "PASSED";
                    case Status::FAILED: return "FAILED";
                    default: return "???";
                }
            }
    };

    namespace {
        std::streambuf* stdcout = nullptr;
        std::stringstream capture_stream = std::stringstream();
        void clear_capture_stream() {
            capture_stream.str(std::string());
        }
        std::string get_output(Runner run) {
            clear_capture_stream();
            run();
            std::string output = capture_stream.str();
            return output;
        }
    }

    bool redirect_cout() {
        if(!stdcout) {
            stdcout = std::cout.rdbuf(capture_stream.rdbuf());
            return true;
        }
        return false;
    }

    bool release_cout() {
        if(stdcout) {
            std::cout.rdbuf(stdcout);
            stdcout = nullptr;
            return true;
        }
        return false;
    }

    std::vector<Test>* get_tests() {
        static std::vector<Test>* p = new std::vector<Test>();
        return p;
    }

    int register_test(Test test) {
        get_tests()->push_back(test);
        return get_tests()->size();
    }

    class AssertionError final : public virtual std::exception {
        private:
            const std::string message;
            const Position position;
        public:
            AssertionError(const std::string& message, const Position position) : message(message), position(position) {}
            ~AssertionError() {}
            virtual const char* what() const noexcept override {
                return message.c_str();
            }

            Position get_position() const noexcept {
                return this->position;
            }
    };

    void assert(bool value, const char* message, Position position) {
        if(!value) {
            throw AssertionError(std::string(message), position);
        }
    }

    void assert_prints(Runner run, const char* text, Position position) {
        std::string output = get_output(run);
        assert(output == text, (std::string("Got output \"") + output + "\", but expected \"" + text + "\"").c_str(), position);
    }

    void assert_print_matches(Runner run, const char* pattern, Position position) {
        std::regex re(pattern);
        std::string output = get_output(run);
        assert(std::regex_search(output, re), (std::string("Got output \"") + output + "\", but expected it to match /" + pattern + "/").c_str(), position);
    }
}

#define __TEST__(NAME, BODY, ID) const int CAT(CAT(__register_test_,NAME),__) = __TestRegistry__::register_test(__TestRegistry__::Test(#NAME, []()->void {BODY;}))
#define TEST(NAME, BODY) __TEST__(NAME, BODY, __COUNTER__)

#define CAT(a,b) __CAT__(a,b)
#define __CAT__(a,b) a##b

#define TASSERT(x) __TestRegistry__::assert(x, #x, __TestRegistry__::Position {__LINE__, __FILE__})
#define TPRINTS(BODY,TEXT) __TestRegistry__::assert_prints([]()->void {BODY;}, TEXT, __TestRegistry__::Position {__LINE__, __FILE__})
#define TPRINTMATCHES(BODY,PATTERN) __TestRegistry__::assert_print_matches([]()->void {BODY;}, PATTERN, __TestRegistry__::Position {__LINE__, __FILE__})

int TEST_MAIN(int argc, char* argv[]) {
    std::cerr << "Testing..." << std::endl;
    std::vector<std::string> arguments(argv + 1, argv + argc);
    int passed = 0, skipped = 0;
    __TestRegistry__::redirect_cout();
    for(__TestRegistry__::Test &test : *__TestRegistry__::get_tests()) {
        if(arguments.empty() || std::find(arguments.begin(), arguments.end(), test.getName()) != arguments.end()) {
            try {
                test.run();
                test.pass();
                passed++;
            } catch(__TestRegistry__::AssertionError &e) {
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
    __TestRegistry__::release_cout();
    std::cerr << std::endl << "=== Test Results ===" << std::endl;
    for(__TestRegistry__::Test &test : *__TestRegistry__::get_tests()) {
        std::cerr << test.getName() << ": " << test.getStatus();
        if(!test.getMessage().empty()) {
            std::cerr << " (" << test.getMessage()  << ")";
        }
        std::cerr << std::endl;
    }
    std::cerr << "====================" << std::endl << std::endl;
    std::cerr << __TestRegistry__::get_tests()->size() << " Total" << std::endl;
    std::cerr << skipped << " SKIPPED" << std::endl;
    std::cerr << passed << " PASSED" << std::endl;
    int failed = __TestRegistry__::get_tests()->size() - skipped - passed;
    std::cerr << failed << " FAILED!" << std::endl;
    return failed ? 1 : 0;
}

#else
#define TEST(...)
#endif
