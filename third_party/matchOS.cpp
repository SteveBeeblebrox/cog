/**
 * @file matchOS.cpp
 * @author Trevor Robinson
 * @brief Makefile code to define macros depending on host OS
 * @version 1.0
 * @date 2012-08-23
 * 
 * @copyright Copyright (c) 2012 - CC BY-SA 3.0 (https://creativecommons.org/licenses/by-sa/3.0/)
 * 
 * Adapted from https://stackoverflow.com/a/12099167 with slight variable renaming and c++ string wrapper
 */

#include <string>
#include "matchOS.h"

/// @brief Makefile code to define macros depending on host OS
const std::string MAKE_MATCH_OS = 
R"""(
ifeq ($(OS),Windows_NT)
    CFLAGS += -D WIN32
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CFLAGS += -D AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CFLAGS += -D AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CFLAGS += -D IA32
        endif
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CFLAGS += -D LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        CFLAGS += -D OSX
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CFLAGS += -D AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CFLAGS += -D IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CFLAGS += -D ARM
    endif
endif
)""";