SDK_PATH := ../../../../..

include $(SDK_PATH)/ThirdParty/PSCommon/BuildSystem/CommonDefs.mak

BIN_DIR = $(SDK_PATH)/Bin

INC_DIRS = \
  . \
  $(SDK_PATH)/Include \
  $(SDK_PATH)/ThirdParty/PSCommon/XnLib/Include

SRC_FILES = *.cpp

USED_LIBS += OpenNI2

LIB_NAME = OpenNI2.jni

CFLAGS += -Wall

include $(SDK_PATH)/ThirdParty/PSCommon/BuildSystem/CommonCppMakefile
