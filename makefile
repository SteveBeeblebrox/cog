# THE NAME OF THE EXECUTABLE
TARGET = cog

SRC_FILES = configstring/classes/ConfigObject.cpp configstring/classes/values.cpp configstring/stringlib.cpp configstring/configstring.cpp main.cpp files.cpp console.cpp commands.cpp formatting.cpp third_party/matchOS.cpp confighelper.cpp actions.cpp

CXX = g++
CFLAGS = -Wall -g -std=c++17
LFLAGS = 

ifeq ($(exprfs), true)
    LFLAGS += -lstdc++fs
    CFLAGS += -DUSE_EXPR_FILESYSTEM
endif

ifeq ($(ansif), true)
    CFLAGS += -DANSI_FORMATTING
endif

OBJECTS = $(SRC_FILES:.cpp=.o)

ifeq ($(shell echo "Windows"), "Windows")
	TARGET := $(TARGET).exe
	DEL = del
	CFLAGS += -DWINDOWS
else
	DEL = rm
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LFLAGS) -o $@ $^

.cpp.o:
	$(CXX) $(CFLAGS) -o $@ -c $<

clean:
	$(DEL) $(TARGET) $(OBJECTS)

# DEPENDENCIES
actions.o: actions.cpp actions.h filesystem.h confighelper.h \
 configstring/configstring.h configstring/stringlib.h \
 configstring/classes/values.hpp configstring/classes/ConfigObject.h \
 configstring/classes/values.hpp console.hpp files.h commands.h \
 formatting.h version.h third_party/matchOS.h
commands.o: commands.cpp commands.h console.hpp configstring/stringlib.h
confighelper.o: confighelper.cpp confighelper.h \
 configstring/configstring.h configstring/stringlib.h \
 configstring/classes/values.hpp configstring/classes/ConfigObject.h \
 configstring/classes/values.hpp filesystem.h files.h console.hpp \
 commands.h
console.o: console.cpp console.hpp
files.o: files.cpp files.h filesystem.h console.hpp commands.h
formatting.o: formatting.cpp formatting.h
main.o: main.cpp console.hpp formatting.h version.h actions.h \
 filesystem.h confighelper.h configstring/configstring.h \
 configstring/stringlib.h configstring/classes/values.hpp \
 configstring/classes/ConfigObject.h configstring/classes/values.hpp \
 files.h third_party/matchOS.h