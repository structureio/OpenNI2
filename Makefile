#############################################################################
# OpenNI makefile.
# 
# default configuration is Release. for a debug version use:
# 	make CFG=Debug
#
# default compiler is g++. for another one use:
#   make CXX=<comp>
#
# By default, CLR projects will only be build if mono is installed.
# To force CLR projects use:
#   make FORCE_BUILD_CLR=1
#
#############################################################################
export ALLOW_WARNINGS = 1

include ThirdParty/PSCommon/BuildSystem/CommonDefs.mak

MAJOR_VERSION = $(shell grep "define ONI_VERSION_MAJOR" Include/OniVersion.h | cut -f 2)
MINOR_VERSION = $(shell grep "define ONI_VERSION_MINOR" Include/OniVersion.h | cut -f 2)
MAINT_VERSION = $(shell grep "define ONI_VERSION_MAINT" Include/OniVersion.h | cut -f 2)

ifeq ("$(OSTYPE)","Darwin")
	OS_NAME = MacOSX
else
	OS_NAME = Linux
endif
PRODUCT_STRING = OpenNI-$(OS_NAME)-$(PLATFORM)-$(shell cd Packaging && python -c "import UpdateVersion; print UpdateVersion.getVersionName()" && cd ..)
FINAL_DIR = Packaging/Final

OPENNI = Source/Core
XNLIB  = ThirdParty/PSCommon/XnLib/Source
DEPTH_UTILS = Source/DepthUtils

# list all drivers
ALL_DRIVERS = \
	Source/Drivers/DummyDevice   \
	Source/Drivers/PS1080 \
	Source/Drivers/PSLink \
	Source/Drivers/OniFile \
	Source/Drivers/Libfreenect2

# list all wrappers
ALL_WRAPPERS = \
	Wrappers/java/OpenNI.jni \
	Wrappers/java/OpenNI.java 

# list all tools
ALL_TOOLS = \
	Source/Drivers/PS1080/PS1080Console \
	Source/Drivers/PSLink/PSLinkConsole

# list all core projects
ALL_CORE_PROJS = \
	$(XNLIB)  \
	$(OPENNI) \
	$(DEPTH_UTILS) \
	$(ALL_DRIVERS) \
	$(ALL_WRAPPERS) \
	$(ALL_TOOLS)

# list all samples
CORE_SAMPLES = \
	Samples/SimpleRead \
	Samples/EventBasedRead \
	Samples/MultipleStreamRead \
	Samples/MWClosestPoint \
	Samples/MWClosestPointApp 

# list all java samples
JAVA_SAMPLES = \
	Samples/SimpleViewer.java	

ifeq "$(GLUT_SUPPORTED)" "1"
	ALL_TOOLS += \
		Source/Tools/NiViewer

	CORE_SAMPLES += \
		Samples/SimpleViewer \
		Samples/MultiDepthViewer \
		Samples/ClosestPointViewer
else
	ifeq "$(GLES_SUPPORTED)" "1"
		CORE_SAMPLES += 
	endif
endif

ALL_SAMPLES = \
	$(CORE_SAMPLES) \
	$(JAVA_SAMPLES)

# list all projects that are build
ALL_BUILD_PROJS = \
	$(ALL_CORE_PROJS) \
	$(ALL_SAMPLES)

ALL_PROJS = \
	$(ALL_BUILD_PROJS)

ALL_PROJS_CLEAN = $(foreach proj,$(ALL_PROJS),$(proj)-clean)

# define a function which creates a target for each proj
define CREATE_PROJ_TARGET
$1: 
	$$(MAKE) -C $1

$1-clean: 
	$$(MAKE) -C $1 clean
endef

################ TARGETS ##################

.PHONY: all $(ALL_PROJS) $(ALL_PROJS_CLEAN) install uninstall clean release

# make all makefiles
all: $(ALL_PROJS)

core: $(ALL_CORE_PROJS)

samples: $(ALL_SAMPLES)

# create projects targets
$(foreach proj,$(ALL_PROJS),$(eval $(call CREATE_PROJ_TARGET,$(proj))))

# additional dependencies
$(OPENNI):                            $(XNLIB)
Wrappers/java/OpenNI.jni:   $(OPENNI) $(XNLIB)

Source/Drivers/DummyDevice: $(OPENNI) $(XNLIB)
Source/Drivers/RawDevice:   $(OPENNI) $(XNLIB)
Source/Drivers/PS1080:      $(OPENNI) $(XNLIB) $(DEPTH_UTILS)
Source/Drivers/PS1080/PS1080Console: $(OPENNI) $(XNLIB)
Source/Drivers/PSLink:      $(OPENNI) $(XNLIB)
Source/Drivers/PSLink/PSLinkConsole: $(OPENNI) $(XNLIB)
Source/Drivers/OniFile:     $(OPENNI) $(XNLIB)

Source/Tools/NiViewer:      $(OPENNI) $(XNLIB)

Samples/SimpleRead:         $(OPENNI)
Samples/EventBasedRead:     $(OPENNI)
Samples/MultipleStreamRead: $(OPENNI)
Samples/MWClosestPoint:     $(OPENNI)
Samples/MWClosestPointApp:  $(OPENNI) Samples/MWClosestPoint

Samples/SimpleViewer:       $(OPENNI)
Samples/MultiDepthViewer:   $(OPENNI)
Samples/ClosestPointViewer: $(OPENNI) Samples/MWClosestPoint
Samples/SimpleViewer.java:            Wrappers/java/OpenNI.java

$(FINAL_DIR):
	mkdir -p $(FINAL_DIR)

doc:
	Source/Documentation/Runme.py
	rm -f Source/Documentation/html/*.md5

release: | all doc $(FINAL_DIR)
	Packaging/Harvest.py Packaging/$(PRODUCT_STRING) $(PLATFORM)
	cd Packaging; tar -cjf Final/$(PRODUCT_STRING).tar.bz2 $(PRODUCT_STRING)

# clean is cleaning all projects
clean: $(ALL_PROJS_CLEAN)
