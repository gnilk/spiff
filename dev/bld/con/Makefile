##########################################################################
##########################################################################
#
# Main make file for mini measurement framework, application and plugins
#
# -depends
#	boost
#	C++11
#
# Compiler settings
#

CC     = clang++

OBJDIR 	 = obj

ROOT		 = ../..
EXT_ROOT	 = $(ROOT)/ext

NXCORE_SRC = $(ROOT)/src
NXCORE_INC = $(ROOT)/inc
DPCOM_SRC = $(EXT_ROOT)/icom
DPCOM_INC = $(EXT_ROOT)/icom

SRC_DIR = $(ROOT)/src
INC_DIR = $(ROOT)/inc

PLUGIN_SRC_DIR = $(ROOT)/plugins

SERVICES_SRC_DIR = $(PLUGIN_SRC_DIR)/services
PLUGIN_DEST_DIR = plugins
PLUGINS = 
PLUGINS_DIRS = \
	$(SERVICES_SRC_DIR)/example \
	$(SERVICES_SRC_DIR)/ping 	\
	$(SERVICES_SRC_DIR)/ftp 	\


INCLUDE_FILES = -I/opt/local/include -I/usr/local/include -I. -I$(INC_DIR) -I$(NXCORE_INC)
LIB_FILES = -framework Cocoa -framework IOKit -framework CoreVideo -L/opt/local/lib

CFLAGS = 
CPPFLAGS = 

# ifeq ($(BUILD),debug) 
# 	CFLAGS = -O0 -g
# 	CPPFLAGS = -O0 -g
# else
# 	CFLAGS = -O2 -DNDEBUG
# 	CPPFLAGS = -O2 -DNDEBUG
# endif



CPPFLAGS = -arch x86_64 -stdlib=libc++ -std=c++11
CPPFLAGS += -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk
CPPFLAGS += $(INCLUDE_FILES)

CFLAGS = -arch x86_64 -std=c99
CFLAGS += -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk
CFLAGS += $(INCLUDE_FILES)



# Linker settings
LD = clang++
LDFLAGS =  -dynamiclib -arch x86_64 -stdlib=libc++
LDFLAGS += $(LIB_FILES)



LINK_LIBS += -framework Cocoa 
NP_LINK_LIBS = 
NXCORE_LINK_LIBS += -L. -lutils
MAIN_LINK_LIBS += $(LIB_FILES) -L. -lnxcore -lutils
BASS_LIB = $(BASS_DIR)/libbass.dylib

MAIN_SRC_FILES = \
	conapp.cpp


NXCORE_SRC_FILES = \
	$(NXCORE_SRC)/prototype.cpp \
	$(NXCORE_SRC)/core/timer.cpp \
	$(NXCORE_SRC)/core/system.cpp \
	$(NXCORE_SRC)/core/config.cpp \
	$(NXCORE_SRC)/messagerouter/filewriter.cpp \
	$(NXCORE_SRC)/messagerouter/router.cpp \
	$(NXCORE_SRC)/services/services.cpp \
	$(NXCORE_SRC)/services/servicemanager.cpp \
	$(NXCORE_SRC)/services/servicedefinition.cpp \
	$(NXCORE_SRC)/services/serviceinstancebridge.cpp \
	$(NXCORE_SRC)/services/serialization/definitionfromxml.cpp \
	$(NXCORE_SRC)/scripting/scriptengine.cpp

UTILS_SRC_FILES =\
	$(NXCORE_SRC)/utils/logger.cpp \
	$(NXCORE_SRC)/utils/pluginscanner.cpp \
	$(NXCORE_SRC)/utils/tokenizer.cpp \
	$(NXCORE_SRC)/utils/inifile.cpp \
	$(NXCORE_SRC)/utils/process.cpp \
	$(NXCORE_SRC)/utils/xmlparser.cpp



NXCORE_OBJ_FILES := $(patsubst %.cpp,%.o,$(NXCORE_SRC_FILES))
UTILS_OBJ_FILES := $(patsubst %.cpp,%.o,$(UTILS_SRC_FILES))
MAIN_OBJ_FILES := $(patsubst %.cpp,%.o,$(MAIN_SRC_FILES))

# Default: Build all tests
all: utils nxcore conapp plugins

.PHONY: plugins $(PLUGINS_DIRS)

 %.o : %.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@

%.o : %.c
	$(CC) -c $(CPPFLAGS)  $< -o $@


conapp: $(MAIN_OBJ_FILES)
	$(CC) $(CFLAGS) $(MAIN_OBJ_FILES) $(MAIN_LINK_LIBS) -o conapp

nxcore: $(NXCORE_OBJ_FILES)
	echo $(NXCORE_OBJ_FILES)
	$(LD) $(LDFLAGS) $(NXCORE_OBJ_FILES) $(NXCORE_LINK_LIBS) -o libnxcore.a

utils: $(UTILS_OBJ_FILES)
	$(LD) $(LDFLAGS) $(UTILS_OBJ_FILES) -o libutils.a

## - compile all plugins by calling recursive make's
plugins: $(PLUGINS_DIRS)
$(PLUGINS_DIRS):
	$(MAKE) -C $@ PLUGIN_NAME=$(@F)
	cp $@/lib$(@F).dylib $(PLUGIN_DEST_DIR)

clean: cleanutils cleannxcore cleanplugins
	rm $(MAIN_OBJ_FILES)
	rm conapp
	echo cleaned

cleanutils:
	rm $(UTILS_OBJ_FILES) libutils.a 

cleannxcore:
	rm $(NXCORE_OBJ_FILES) libnxcore.a 


cleanplugins:
	rm $(PLUGIN_DEST_DIR)/*.dylib
	for dir in $(PLUGINS_DIRS); do\
		$(MAKE) -C $$dir clean; \
	done
