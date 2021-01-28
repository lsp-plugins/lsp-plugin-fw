#
# Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
#
# This file is part of lsp-tk-lib
#
# lsp-tk-lib is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# lsp-tk-lib is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with lsp-tk-lib.  If not, see <https://www.gnu.org/licenses/>.
#

BASEDIR                 = $(CURDIR)
CONFIG                 := $(CURDIR)/.config.mk
PROJECT                := $(CURDIR)/../project.mk
PLUGINS                := $(CURDIR)/../plugins.mk

include $(CONFIG)
include $(PLUGINS)
include $(PROJECT)
include $(BASEDIR)/../make/functions.mk

ifeq ($(TEST),1)
  UNIQ_DEPENDENCIES      := $(call uniq, $(DEPENDENCIES) $(TEST_DEPENDENCIES))
else
  UNIQ_DEPENDENCIES      := $(call uniq, $(DEPENDENCIES))
endif
BIN_DEPENDENCIES        = $(foreach dep,$(UNIQ_DEPENDENCIES),$(if $(findstring bin,$($(dep)_TYPE)),$(dep)))
DEPENDENCIES            = $(filter-out $(BIN_DEPENDENCIES), $(UNIQ_DEPENDENCIES))

#------------------------------------------------------------------------------
# Framework settings
LSP_PLUGIN_FW_OBJ_CORE      = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-core.o
LSP_PLUGIN_FW_OBJ_META      = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-meta.o
LSP_PLUGIN_FW_OBJ_DSP       = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-dsp.o
LSP_PLUGIN_FW_OBJ_UI        = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui.o
LSP_PLUGIN_FW_OBJ_TEST      = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-test.o
LSP_PLUGIN_FW_OBJ           = $(LSP_PLUGIN_FW_OBJ_CORE) $(LSP_PLUGIN_FW_OBJ_META) $(LSP_PLUGIN_FW_OBJ_DSP) $(LSP_PLUGIN_FW_OBJ_UI)
LSP_PLUGIN_FW_MFLAGS       += $(foreach dep,$(DEPENDENCIES),-DUSE_$(dep)) 
LSP_PLUGIN_FW_CFLAGS       += $(foreach dep,$(DEPENDENCIES), $($(dep)_CFLAGS))

# Source code location
CXX_SRC_CORE                = $(call rwildcard, main/core, *.cpp)
CXX_SRC_META                = $(call rwildcard, main/meta, *.cpp)
CXX_SRC_DSP                 = $(call rwildcard, main/plug, *.cpp)
CXX_SRC_UI                  = $(call rwildcard, main/ui, *.cpp)
CXX_SRC_WRAP_JACK           = wrap/jack.cpp
CXX_SRC_UTIL                = $(call rwildcard, util, *.cpp)
CXX_SRC_TEST                = $(call rwildcard, test, *.cpp)
CXX_SRC                     = \
  $(CXX_SRC_CORE) \
  $(CXX_SRC_META) \
  $(CXX_SRC_DSP) \
  $(CXX_SRC_UI) \
  $(CXX_SRC_WRAP_JACK) \
  $(CXX_SRC_UTIL)

# Object files
OBJ_STUB                    = $(patsubst %.cpp, %.o, $(CXX_SRC_STUB))
OBJ_CORE                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_CORE))
OBJ_META                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_META))
OBJ_DSP                     = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_DSP))
OBJ_UI                      = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_UI))
OBJ_WRAP_JACK               = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_JACK))
OBJ_UTIL                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_UTIL))
OBJ_TEST                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_TEST))
OBJ                         = \
  $(OBJ_CORE) \
  $(OBJ_META) \
  $(OBJ_DSP) \
  $(OBJ_UI) \
  $(OBJ_UTIL) \
  $(OBJ_WRAP_JACK)

ifeq ($(TEST),1)
  CXX_SRC                    += $(CXX_SRC_TEST)
  OBJ                        += $(OBJ_TEST)
  LSP_PLUGIN_FW_OBJ          += $(LSP_PLUGIN_FW_OBJ_TEST)
endif

#------------------------------------------------------------------------------
# Utilities
UTL_BIN_PATH                = $(ARTIFACT_BIN)/util
UTL_JACK_MAKE               = $(UTL_BIN_PATH)/jack_make$(EXECUTABLE_EXT)
UTL_JACK_MAKE_OBJ           = $(UTL_BIN_PATH)/jack_make.o
UTL_JACK_MAKE_DEPS          = $(foreach dep, $(call uniq, $(DEPENDENCIES)), $(if $($(dep)_OBJ), $(dep)))
UTL_JACK_MAKE_LIBS          = $(foreach dep, $(call uniq, $(DEPENDENCIES)), $($(dep)_OBJ))
UTL_JACK_MAKE_LDFLAGS       = $(foreach dep, $(call uniq, $(DEPENDENCIES)), $($(dep)_LDFLAGS))
UTL_JACK_MAKE_OBJS          = $(LSP_PLUGIN_FW_OBJ_CORE) $(LSP_PLUGIN_FW_OBJ_META) $(LSP_PLUGIN_FW_OBJ_DSP) \
                              $(OBJ_PLUG_META) $(OBJ_PLUG_DSP)

#------------------------------------------------------------------------------
# Plugins
OBJ_PLUG_META               = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_META))
OBJ_PLUG_DSP                = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_DSP))
OBJ_PLUG_UI                 = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_UI))
OBJ_PLUG_TEST               = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_TEST))
OBJ_PLUG                    = $(OBJ_PLUG_META) $(OBJ_PLUG_DSP) $(OBJ_PLUG_UI)

#------------------------------------------------------------------------------
# Binaries
ARTIFACT_BIN                = $($(ARTIFACT_ID)_BIN)

ARTIFACT_LIB_JACK           = $(ARTIFACT_BIN)/lib$(ARTIFACT_NAME)-jack-$(ARTIFACT_VERSION)$(LIBRARY_EXT)
ARTIFACT_LIB_JACK_PATH      = $(ARTIFACT_BIN)/jack
ARTIFACT_LIB_JACK_SETTINGS  = $(ARTIFACT_LIB_JACK_PATH)/.settings.mk
ARTIFACT_LIB_JACK_DEPS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI)), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_JACK_LIBS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI)), $($(dep)_OBJ))
ARTIFACT_LIB_JACK_LDFLAGS   = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI)), $($(dep)_LDFLAGS))
ARTIFACT_LIB_JACK_OBJS      = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_UI) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_UI) \
  $(OBJ_WRAP_JACK)

BIN_INSTALL                 = $(foreach dep, $(BIN_DEPENDENCIES), install_$(dep))
BIN_ARTIFACT_ID             = $(patsubst install_%,%,$(@))

#------------------------------------------------------------------------------
# Nested variables
SRC_DEPS                    = $(foreach dep, $(DEPENDENCIES), $(if $(findstring src,$($(dep)_TYPE)),$(dep)))
PLUG_DEPS                   = $(foreach dep, $(DEPENDENCIES) $(PLUGIN_DEPENDENCIES), $(if $(findstring plug,$($(dep)_TYPE)),$(dep)))

#------------------------------------------------------------------------------
# Functional variables
CXX_FILE                    = $(patsubst $(LSP_PLUGIN_FW_BIN)/%.o,%.cpp, $(@))

#------------------------------------------------------------------------------
# Plugin instance settings
PLUGIN_PACKAGE_MAJOR       := $(call vmajor,$(PLUGIN_PACKAGE_VERSION))
PLUGIN_PACKAGE_MINOR       := $(call vminor,$(PLUGIN_PACKAGE_VERSION))
PLUGIN_PACKAGE_MICRO       := $(call vmicro,$(PLUGIN_PACKAGE_VERSION))
PLUGIN_PACKAGE_BRANCH      := $(call vbranch,$(PLUGIN_PACKAGE_VERSION))

WRAP_JACK_CFLAGS           := \
  -DINSTALL_PREFIX=\"$(PREFIX)\" \
  -DPLUGIN_PACKAGE_MAJOR=$(PLUGIN_PACKAGE_MAJOR) \
  -DPLUGIN_PACKAGE_MINOR=$(PLUGIN_PACKAGE_MINOR) \
  -DPLUGIN_PACKAGE_MICRO=$(PLUGIN_PACKAGE_MICRO) \
  -DPLUGIN_PACKAGE_BRANCH=\"$(PLUGIN_PACKAGE_BRANCH)\" \
  -DLSP_ARTIFACT_ID=\"$($(ARTIFACT_ID)_NAME)\"
WRAP_JACK_LDFLAGS          := $(call query,LDFLAGS,$(DEPENDENCIES_JACK_WRAP))
WRAP_JACK_OBJS             := $(call query,OBJ,$(DEPENDENCIES_JACK_WRAP))
WRAP_JACK_INC              := $(call query,CFLAGS,$(DEPENDENCIES_JACK_WRAP) LSP_PLUGIN_FW)

.DEFAULT_GOAL = all
.PHONY: compile depend dep_clean all install uninstall
.PHONY: jack ladspa dssi lv2 vst
.PHONY: $(DEPENDENCIES)
.PHONY: $(BIN_INSTALL)

#------------------------------------------------------------------------------
# Dependencies
#dep_clean:
#
#$(CXX_DEP_ALL): dep_clean
#	@echo "  dep  [$(ARTIFACT_NAME)] $(CXX_DEP_FILE)"
#	@mkdir -p $(dir $(@))
#	$(CXX) -MM -MT "\$$(ARTIFACT_BIN)/$(CXX_DEP_TARGET)" -MF $(@) $(CXX_DEP_FILE) $(CXXFLAGS) $(INCLUDE) $(CFLAGS_DEPS)
#
#depend: $(CXX_DEP_ALL)
#	$(foreach dep,$(DEPENDENCIES) $(ARTIFACT_ID),\
#	  $(if $($(dep)_INC), \
#	    $(eval SED_RULES += s/$(shell echo "$($(dep)_INC)" | sed "s/\\//\\\\\//g")/\$$$$\\($(dep)_INC\\)/g;)\
#	  )\
#	  $(if $($(dep)_BIN), \
#	    $(eval SED_RULES += s/$(shell echo "$($(dep)_BIN)" | sed "s/\\//\\\\\//g")/\\$$$$\\($(dep)_BIN\\)/g;)\
#	  )\
#	)
#	@cat $(CXX_DEP_ALL) | sed -E "$(SED_RULES)" >Makefile.d
#

$(SRC_DEPS) $(PLUG_DEPS):
	@echo "make [$($(@)_NAME)]"
	@echo $(MAKE) -s -C "$($(@)_PATH)" compile PROJECT="$(PROJECT)" CONFIG="$(CONFIG)" ARTIFACT_ID="$(@)" NO_DEPENDENCIES="1" DEMO_TEST="0"
	@$(MAKE) -s -C "$($(@)_PATH)" compile PROJECT="$(PROJECT)" CONFIG="$(CONFIG)" ARTIFACT_ID="$(@)" NO_DEPENDENCIES="1" DEMO_TEST="0"

$(BIN_DEPENDENCIES):
	@echo "make [$($(@)_NAME)]"
	@$(MAKE) -s -C "$($(@)_PATH)" all CONFIG="$(CONFIG)" DEMO_TEST="0"

#------------------------------------------------------------------------------
# Compilation
$(OBJ):
	@echo "  $(CXX)  [$(LSP_PLUGIN_FW_NAME)] $(CXX_FILE)"
	@mkdir -p $(dir $@)
	@echo $(CXX) -o $(@) -c $(CXX_FILE) -fPIC $(CXXFLAGS) $(LSP_PLUGIN_FW_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_CFLAGS)
	@$(CXX) -o $(@) -c $(CXX_FILE) -fPIC $(CXXFLAGS) $(LSP_PLUGIN_FW_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_CFLAGS)

$(OBJ_WRAP_JACK): EXT_FLAGS=$(WRAP_JACK_CFLAGS)

#compile:

#------------------------------------------------------------------------------
# Linking targets
$(LSP_PLUGIN_FW_OBJ_CORE): $(OBJ_CORE)
	@echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_CORE))"
	@$(LD) -o $(LSP_PLUGIN_FW_OBJ_CORE) -r $(OBJ_CORE)
	
$(LSP_PLUGIN_FW_OBJ_META): $(OBJ_META)
	@echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_META))"
	@$(LD) -o $(LSP_PLUGIN_FW_OBJ_META) -r $(OBJ_META)
	
$(LSP_PLUGIN_FW_OBJ_DSP): $(OBJ_DSP)
	@echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_DSP))"
	@$(LD) -o $(LSP_PLUGIN_FW_OBJ_DSP) -r $(OBJ_DSP)
	
$(LSP_PLUGIN_FW_OBJ_UI): $(OBJ_UI)
	@echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_UI))"
	@$(LD) -o $(LSP_PLUGIN_FW_OBJ_UI) -r $(OBJ_UI)
	
$(LSP_PLUGIN_FW_OBJ_TEST): $(OBJ_TEST)
	@echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_TEST))"
	@$(LD) -o $(LSP_PLUGIN_FW_OBJ_TEST) -r $(OBJ_TEST)

#------------------------------------------------------------------------------
# Utilities
$(UTL_JACK_MAKE): $(LSP_PLUGIN_FW_OBJ) $(UTL_JACK_MAKE_OBJ) $(UTL_JACK_MAKE_DEPS) $(PLUG_DEPS)
	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(UTL_JACK_MAKE))"
	@mkdir -p $(UTL_BIN_PATH)
	@echo $(CXX) -o $(UTL_JACK_MAKE) $(UTL_JACK_MAKE_OBJ) $(UTL_JACK_LIBS) $(UTL_JACK_MAKE_OBJS) $(UTL_JACK_MAKE_LIBS) $(EXE_FLAGS) $(UTL_JACK_MAKE_LDFLAGS)
	@$(CXX) -o $(UTL_JACK_MAKE) $(UTL_JACK_MAKE_OBJ) $(UTL_JACK_LIBS) $(UTL_JACK_MAKE_OBJS) $(UTL_JACK_MAKE_LIBS) $(EXE_FLAGS) $(UTL_JACK_MAKE_LDFLAGS)

#------------------------------------------------------------------------------
# Binaries
$(ARTIFACT_LIB_JACK): $(LSP_PLUGIN_FW_OBJ) $(OBJ_WRAP_JACK) $(ARTIFACT_LIB_JACK_DEPS) $(PLUG_DEPS)
	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_JACK))"
	@echo $(CXX) -o $(ARTIFACT_LIB_JACK) $(ARTIFACT_LIB_JACK_LIBS) $(ARTIFACT_LIB_JACK_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_JACK_LDFLAGS)
	@$(CXX) -o $(ARTIFACT_LIB_JACK) $(ARTIFACT_LIB_JACK_LIBS) $(ARTIFACT_LIB_JACK_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_JACK_LDFLAGS)

# All targets
jack: $(UTL_JACK_MAKE) $(ARTIFACT_LIB_JACK)
	@echo "  $(notdir $(UTL_JACK_MAKE)) [$(ARTIFACT_NAME)] $(patsubst $(ARTIFACT_BIN)/%,%, $(ARTIFACT_LIB_JACK_PATH))"
	@mkdir -p "$(ARTIFACT_LIB_JACK_PATH)"
	@$(UTL_JACK_MAKE) "$(ARTIFACT_LIB_JACK_PATH)"
	@echo "  make JACK"
	@echo -e \
	  "# Auto-generated script, do not edit\n\n" \
	  "EXT_OBJS=$(WRAP_JACK_OBJS)\n" \
	  "EXT_CFLAGS=$(WRAP_JACK_CFLAGS)\n" \
	  "EXT_CXXFLAGS=$(WRAP_JACK_CFLAGS)\n" \
	  "EXT_LDFLAGS=$(WRAP_JACK_LDFLAGS)\n" \
	  "EXT_INCLUDE=$(WRAP_JACK_INC)\n" | \
	  sed -E 's/\"/\\\"/g' > $(ARTIFACT_LIB_JACK_SETTINGS)
	@make -C "$(ARTIFACT_LIB_JACK_PATH)" depend CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"
	@make -C "$(ARTIFACT_LIB_JACK_PATH)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"

all: jack

#------------------------------------------------------------------------------
# Installation
$(BIN_INSTALL):
	@echo "INSTALL $(notdir $($(BIN_ARTIFACT_ID)_PATH))"
	@echo make -C "$($(BIN_ARTIFACT_ID)_PATH)" install DESTDIR="$(DESTDIR)"
	@make -C "$($(BIN_ARTIFACT_ID)_PATH)" install DESTDIR="$(DESTDIR)"

install: $(BIN_INSTALL)
	

# Dependencies
-include $(CURDIR)/Makefile.d


