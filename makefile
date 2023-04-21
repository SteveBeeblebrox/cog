# COMMENTS BEGIN WITH A HASH

# THE NAME OF YOUR EXECUTABLE
TARGET = cfgstr
# ALL CPP COMPILABLE IMPLEMENTATION FILES THAT MAKE UP THE PROJECT
SRC_FILES = configstring/classes/ConfigObject.cpp configstring/classes/values.cpp configstring/stringlib.cpp configstring/configstring.cpp main.cpp files.cpp console.cpp commands.cpp

# NO EDITS NEEDED BELOW THIS LINE

CXX = g++
CFLAGS = -Wall -g -std=c++17

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
	$(CXX) -o $@ $^

.cpp.o:
	$(CXX) $(CFLAGS) -o $@ -c $<

clean:
	$(DEL) $(TARGET) $(OBJECTS)

# DEPENDENCIES
main.o: main.cpp