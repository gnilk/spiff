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
SDK = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk


ROOT		 = ..
EXT_ROOT	 = $(ROOT)/ext

CON_ROOT = console
SPIFF_ROOT = spiff
TT_ROOT = timertest
HAL_ROOT = hal
HALNX_ROOT = hal_linux
BIN_ROOT = bin
BIN_DIR = $(BIN_ROOT)

CONAPP_SRC = console
SPIFF_SRC = $(SPIFF_ROOT)
TT_SRC = $(TT_ROOT)
HAL_SRC = $(HAL_ROOT)
HALNX_SRC = $(HALNX_ROOT)
NXCORE_SRC = $(ROOT)/src
NXCORE_INC = $(ROOT)/inc
DPCOM_SRC = $(EXT_ROOT)/icom
DPCOM_INC = $(EXT_ROOT)/icom

SRC_DIR = $(ROOT)/src
INC_DIR = $(ROOT)/inc

PLUGIN_SRC_DIR = $(ROOT)/plugins

SERVICES_SRC_DIR = $(PLUGIN_SRC_DIR)/services
PLUGIN_DEST_DIR = $(BIN_ROOT)/plugins
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



CPPFLAGS = -g -arch x86_64 -stdlib=libc++ -std=c++11
CPPFLAGS += -isysroot $(SDK)
CPPFLAGS += $(INCLUDE_FILES)

CFLAGS = -g -arch x86_64 -std=c99
CFLAGS += -isysroot $(SDK)
CFLAGS += $(INCLUDE_FILES)



# Linker settings
LD = clang++
LDFLAGS = -dynamiclib -arch x86_64 -stdlib=libc++
LDFLAGS += $(LIB_FILES)



LINK_LIBS += -framework Cocoa -framework SystemConfiguration
NP_LINK_LIBS = 
NXCORE_LINK_LIBS += -L. -lutils
MAIN_LINK_LIBS += $(LIB_FILES) -L. -lnxcore -lutils
BASS_LIB = $(BASS_DIR)/libbass.dylib

CONAPP_SRC_FILES = \
	console/conapp.cpp

SPIFF_SRC_FILES = \
	$(SPIFF_SRC)/spiff.cpp

TT_SRC_FILES = \
	$(TT_SRC)/tt.cpp

HAL_SRC_FILES = \
	$(HAL_SRC)/hal.cpp

HALNX_SRC_FILES = \
	$(HALNX_SRC)/hal_linux.cpp \
	$(HALNX_SRC)/context_test1.cpp \
	$(HALNX_SRC)/context_test2.cpp \
	$(HALNX_SRC)/context_test3.cpp 

NXCORE_SRC_FILES = \
	$(NXCORE_SRC)/prototype.cpp \
	$(NXCORE_SRC)/core/timer.cpp \
	$(NXCORE_SRC)/core/event.cpp \
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
CONAPP_OBJ_FILES := $(patsubst %.cpp,%.o,$(CONAPP_SRC_FILES))
SPIFF_OBJ_FILES := $(patsubst %.cpp,%.o,$(SPIFF_SRC_FILES))
TT_OBJ_FILES := $(patsubst %.cpp,%.o,$(TT_SRC_FILES))
HAL_OBJ_FILES := $(patsubst %.cpp,%.o,$(HAL_SRC_FILES))
HALNX_OBJ_FILES := $(patsubst %.cpp,%.o,$(HALNX_SRC_FILES))

# Default: Build all tests
all: utils nxcore conapp spiff tt hal halnx plugins

.PHONY: plugins $(PLUGINS_DIRS)

 %.o : %.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@

%.o : %.c
	$(CC) -c $(CPPFLAGS)  $< -o $@

spiff: $(SPIFF_OBJ_FILES) utils nxcore
	$(CC) $(CFLAGS) $(SPIFF_OBJ_FILES) $(MAIN_LINK_LIBS) -o $(BIN_DIR)/spiff

tt: $(TT_OBJ_FILES) utils nxcore
	$(CC) $(CFLAGS) $(TT_OBJ_FILES) $(MAIN_LINK_LIBS) -o $(BIN_DIR)/tt

hal: $(HAL_OBJ_FILES) utils nxcore
	$(CC) $(CFLAGS) $(HAL_OBJ_FILES) $(MAIN_LINK_LIBS) -o $(BIN_DIR)/hal

halnx: $(HALNX_OBJ_FILES) utils nxcore
	$(CC) $(CFLAGS) $(HALNX_OBJ_FILES) $(MAIN_LINK_LIBS) -o $(BIN_DIR)/hal_linux

conapp: $(CONAPP_OBJ_FILES) utils nxcore
	$(CC) $(CFLAGS) $(CONAPP_OBJ_FILES) $(MAIN_LINK_LIBS) -o $(BIN_DIR)/conapp

nxcore: $(NXCORE_OBJ_FILES) utils
	echo $(NXCORE_OBJ_FILES)
	$(LD) $(LDFLAGS) $(NXCORE_OBJ_FILES) $(NXCORE_LINK_LIBS) -o libnxcore.a
	cp libnxcore.a $(BIN_DIR)/.
	

utils: $(UTILS_OBJ_FILES)
	$(LD) $(LDFLAGS) $(UTILS_OBJ_FILES) -v -o libutils.a
	cp libutils.a $(BIN_DIR)/.

## - compile all plugins by calling recursive make's
plugins: $(PLUGINS_DIRS) utils
$(PLUGINS_DIRS):
	$(MAKE) -C $@ PLUGIN_NAME=$(@F)
	cp $@/lib$(@F).dylib $(PLUGIN_DEST_DIR)

clean: cleanspiff cleanconapp cleanutils cleannxcore cleanplugins cleanhal
	echo cleaned

cleanhal:
	rm $(HAL_OBJ_FILES)
	rm $(HALNX_OBJ_FILES)
	rm $(BIN_DIR)/hal
	rm $(BIN_DIR)/hal_linux

cleanconapp:
	rm $(CONAPP_OBJ_FILES)
	rm $(BIN_DIR)/conapp

cleanspiff:
	rm $(SPIFF_OBJ_FILES)
	rm $(BIN_DIR)/spiff

cleanutils:
	rm $(UTILS_OBJ_FILES) $(BIN_DIR)/libutils.a 
	rm libutils.a

cleannxcore:
	rm $(NXCORE_OBJ_FILES) $(BIN_DIR)/libnxcore.a 
	rm libnxcore.a


cleanplugins:
	rm $(PLUGIN_DEST_DIR)/*.dylib
	for dir in $(PLUGINS_DIRS); do\
		$(MAKE) -C $$dir clean; \
	done
