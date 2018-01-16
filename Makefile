# OpenNI 2 Makefile
#
# Default configuration is Release. for a debug version use:
# 	make CFG=Debug
#
# Default compiler is g++. for another one use:
#   make CXX=<comp>
#
# Java-dependent build rules are disabled by default. To enable them, use:
#   make HAS_JAVA=1

#-------------------------------------------------------------------------------

include ThirdParty/PSCommon/BuildSystem/CommonDefs.mak

#-------------------------------------------------------------------------------

# HAS_JAVA=0 # Uncomment this to force the value.

MAJOR_VERSION = $(shell grep "define ONI_VERSION_MAJOR" Include/OniVersion.h | cut -f 2)
MINOR_VERSION = $(shell grep "define ONI_VERSION_MINOR" Include/OniVersion.h | cut -f 2)
MAINT_VERSION = $(shell grep "define ONI_VERSION_MAINT" Include/OniVersion.h | cut -f 2)

ifeq ("$(OSTYPE)","Darwin")
	OS_NAME = MacOSX
else
	OS_NAME = Linux
endif

PRODUCT_STRING = OpenNI-$(OS_NAME)-$(PLATFORM)-$(shell cd Packaging && python2 -c "import UpdateVersion; print UpdateVersion.getVersionName()" && cd ..)

FINAL_DIR = Packaging/Final

CORE = Source/Core
XNLIB = ThirdParty/PSCommon/XnLib/Source
DEPTH_UTILS = Source/DepthUtils

#-------------------------------------------------------------------------------
# C++

CXX_MAIN_SUBDIRS = \
	$(CORE) \
	ThirdParty/PSCommon/XnLib/Source \
	Source/DepthUtils \
	Source/Drivers/DummyDevice   \
	Source/Drivers/PS1080 \
	Source/Drivers/PSLink \
	Source/Drivers/OniFile \
	Source/Drivers/PS1080/PS1080Console \
	Source/Drivers/PSLink/PSLinkConsole

CXX_SAMPLES_SUBDIRS = \
	Samples/SimpleRead \
	Samples/EventBasedRead \
	Samples/MultipleStreamRead \
	Samples/MWClosestPoint \
	Samples/MWClosestPointApp

#-------------------------------------------------------------------------------
# Java

JAVA_MAIN_SUBDIRS = \
	Wrappers/java/ \
	Wrappers/java/src/main/jni
	# Wrappers/java/src/main/java \

JAVA_SAMPLES_SUBDIRS = \
	Samples/SimpleViewer.java

#-------------------------------------------------------------------------------
# GLUT

ifeq "$(GLUT_SUPPORTED)" "1"
	CXX_MAIN_SUBDIRS += \
		Source/Tools/NiViewer

	CXX_SAMPLES_SUBDIRS += \
		Samples/SimpleViewer \
		Samples/MultiDepthViewer \
		Samples/ClosestPointViewer
endif

#-------------------------------------------------------------------------------
# Target groups

ALL_CXX_SUBDIRS = \
	$(CXX_MAIN_SUBDIRS) \
	$(CXX_SAMPLES_SUBDIRS) \

ALL_JAVA_SUBDIRS = \
	$(JAVA_MAIN_SUBDIRS) \
	$(JAVA_SAMPLES_SUBDIRS)

ALL_MAIN_SUBDIRS = \
	$(CXX_MAIN_SUBDIRS) \
	$(JAVA_MAIN_SUBDIRS)

ALL_SAMPLES_SUBDIRS = \
	$(CXX_SAMPLES_SUBDIRS) \
	$(JAVA_SAMPLES_SUBDIRS)

ALL_SUBDIRS = \
	$(ALL_MAIN_SUBDIRS) \
	$(ALL_SAMPLES_SUBDIRS)

# Add an unconditional shorthand for java targets
java: $(ALL_JAVA_SUBDIRS)

.PHONY: java

#-------------------------------------------------------------------------------
# Recursive make machinery

# Compute the list of cleaning targets.
CLEAN_SUBDIRS = $(foreach target,$(ALL_SUBDIRS),$(target)-clean)

# Define a function for creating per-subdirectory target rules.
define CREATE_SUBDIR
$1:
	$$(MAKE) -C $1

$1-clean:
	$$(MAKE) -C $1 clean
endef

# Create all per-subdirectory targets.
$(foreach target,$(ALL_SUBDIRS),$(eval $(call CREATE_SUBDIR,$(target))))

# Declare all per-subdirectory rules as phony, so that they're always built.
# See: http://www.gnu.org/software/make/manual/make.html#Recursion
.PHONY: \
	$(ALL_SUBDIRS) \
	$(CLEAN_SUBDIRS)

# Set 'all' as the default target, since it is not the first target defined in this Makefile.
.DEFAULT_GOAL = all

#-------------------------------------------------------------------------------
# Additional rules

$(FINAL_DIR):
	mkdir -p $(FINAL_DIR)

#-------------------------------------------------------------------------------
# Target dependencies

$(CORE): $(XNLIB)

Wrappers/java:                        Wrappers/java/src/main/jni
Wrappers/java/src/main/jni:           $(CORE)

Source/Drivers/DummyDevice:           $(CORE)
Source/Drivers/RawDevice:             $(CORE)
Source/Drivers/PS1080:                $(CORE) $(DEPTH_UTILS)
Source/Drivers/PS1080/PS1080Console:  $(CORE)
Source/Drivers/PSLink:                $(CORE)
Source/Drivers/PSLink/PSLinkConsole:  $(CORE)
Source/Drivers/OniFile:               $(CORE)

Source/Tools/NiViewer:                $(CORE)

Samples/SimpleRead:                   $(CORE)
Samples/EventBasedRead:               $(CORE)
Samples/MultipleStreamRead:           $(CORE)
Samples/MWClosestPoint:               $(CORE)
Samples/SimpleViewer:                 $(CORE)
Samples/MultiDepthViewer:             $(CORE)

Samples/MWClosestPointApp:            $(CORE) Samples/MWClosestPoint
Samples/ClosestPointViewer:           $(CORE) Samples/MWClosestPoint

Samples/SimpleViewer.java:            Wrappers/java/src/main/java

#-------------------------------------------------------------------------------
# Top-level targets

MAIN_SUBDIRS = \
	$(CXX_MAIN_SUBDIRS)

SAMPLES_SUBDIRS = \
	$(CXX_SAMPLES_SUBDIRS)

# Add java targets to the default build, depending on the HAS_JAVA variable.
ifeq ($(HAS_JAVA), 1)
	MAIN_SUBDIRS += $(JAVA_MAIN_SUBDIRS)
	SAMPLES_SUBDIRS += $(JAVA_SAMPLES_SUBDIRS)
endif

all: main samples

main: $(MAIN_SUBDIRS)

samples: $(SAMPLES_SUBDIRS)

doc:
	Source/Documentation/Runme.py
	rm -f Source/Documentation/html/*.md5

release: | all doc $(FINAL_DIR)
	Packaging/Harvest.py Packaging/$(PRODUCT_STRING) $(PLATFORM)
	cd Packaging; tar -cjf Final/$(PRODUCT_STRING).tar.bz2 $(PRODUCT_STRING)

clean: $(CLEAN_SUBDIRS)

.PHONY: \
	all \
	doc \
	main \
	samples \
	release \
	clean
