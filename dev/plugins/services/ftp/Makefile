##########################################################################
##########################################################################

# Compiler settings
CC     = g++

OBJDIR 	 = obj

ROOT		 = ../../..
EXT_ROOT	 = $(ROOT)/ext

NXCORE_SRC_DIR = $(ROOT)/src
NXCORE_INC_DIR = $(ROOT)/inc

CFLAGS = -g -arch x86_64 -stdlib=libc++ -std=c++11
#CFLAGS += -macosx_version_min 10.8
CFLAGS += -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk
CFLAGS += -I/opt/local/include -I/usr/local/include -I. -I$(NXCORE_INC_DIR)


# Linker settings
LD = clang++
LDFLAGS =  -dynamiclib -arch x86_64 -stdlib=libc++
LDFLAGS += -framework Cocoa 


PLUGIN_NAME_LIB = lib$(PLUGIN_NAME).dylib
PLUGIN_SRC_FILES = $(wildcard *.cpp)
PLUGIN_OBJ_FILES := $(patsubst %.cpp,%.o,$(PLUGIN_SRC_FILES))

# Default: Build all tests
all: plugin

 %.o : %.cpp
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

plugin: $(PLUGIN_OBJ_FILES)
	$(LD) $(LDFLAGS) $(PLUGIN_OBJ_FILES) -o $(PLUGIN_NAME_LIB)

clean:
	rm $(PLUGIN_OBJ_FILES) *.dylib
