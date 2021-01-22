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

CONFIG                     := $(CURDIR)/.config.mk

include $(CONFIG)
include $(BASEDIR)/make/functions.mk

#------------------------------------------------------------------------------
# Framework settings
LSP_PLUGIN_FW_OBJ_CORE      = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-core.o
LSP_PLUGIN_FW_OBJ_META      = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-meta.o
LSP_PLUGIN_FW_OBJ_DSP       = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-dsp.o
LSP_PLUGIN_FW_OBJ_UI        = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui.o
LSP_PLUGIN_FW_OBJ_TEST      = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-test.o
LSP_PLUGIN_FW_OBJ           = $(LSP_PLUGIN_FW_OBJ_CORE) $(LSP_PLUGIN_FW_OBJ_META) $(LSP_PLUGIN_FW_OBJ_DSP) $(LSP_PLUGIN_FW_OBJ_UI)
LSP_PLUGIN_FW_MFLAGS       += $(foreach dep,$(BUILD_DEPENDENCIES),-DUSE_$(dep))
LSP_PLUGIN_FW_CFLAGS       += $(foreach dep,$(BUILD_DEPENDENCIES), $($(dep)_CFLAGS))

# Source code location
CXX_SRC_CORE                = $(call rwildcard, main/core, *.cpp)
CXX_SRC_META                = $(call rwildcard, main/meta, *.cpp)
CXX_SRC_DSP                 = $(call rwildcard, main/plug, *.cpp)
CXX_SRC_UI                  = $(call rwildcard, main/ui, *.cpp)
CXX_SRC_WRAP                = $(call rwildcard, main/wrap, *.cpp)
CXX_SRC_UTIL                = $(call rwildcard, util, *.cpp)
CXX_SRC_TEST                = $(call rwildcard, test, *.cpp)
CXX_SRC                     = $(CXX_SRC_CORE) $(CXX_SRC_META) $(CXX_SRC_DSP) $(CXX_SRC_UI) $(CXX_SRC_WRAP) $(CXX_SRC_UTIL)

# Object files
OBJ_STUB                    = $(patsubst %.cpp, %.o, $(CXX_SRC_STUB))
OBJ_CORE                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_CORE))
OBJ_META                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_META))
OBJ_DSP                     = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_DSP))
OBJ_UI                      = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_UI))
OBJ_UTIL                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_UTIL))
OBJ_TEST                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_TEST))
OBJ                         = $(OBJ_CORE) $(OBJ_META) $(OBJ_DSP) $(OBJ_UI) $(OBJ_UTIL)

ifeq ($(TEST),1)
  CXX_SRC                    += $(CXX_SRC_TEST)
  OBJ                        += $(OBJ_TEST)
  LSP_PLUGIN_FW_OBJ          += $(LSP_PLUGIN_FW_OBJ_TEST)
endif

# Utilities
UTL_BIN_PATH                = $(ARTIFACT_BIN)/util
UTL_JACK_MAKE               = $(UTL_BIN_PATH)/jack_make$(EXECUTABLE_EXT)
UTL_JACK_MAKE_OBJ           = $(UTL_BIN_PATH)/jack_make.o
UTL_JACK_MAKE_DEPS          = $(foreach dep, $(call uniq, $(DEPENDENCIES_COMMON) $(DEPENDENCIES_PLUGINS)), $(if $($(dep)_OBJ), $(dep)))
UTL_JACK_MAKE_LIBS          = $(foreach dep, $(call uniq, $(DEPENDENCIES_COMMON) $(DEPENDENCIES_PLUGINS)), $($(dep)_OBJ))
UTL_JACK_MAKE_LDFLAGS       = $(foreach dep, $(call uniq, $(DEPENDENCIES_COMMON) $(DEPENDENCIES_PLUGINS)), $($(dep)_LDFLAGS))
UTL_JACK_MAKE_OBJS          = $(ARTIFACT_OBJ_CORE) $(ARTIFACT_OBJ_META) $(ARTIFACT_OBJ_DSP) \
                              $(OBJ_PLUG_META) $(OBJ_PLUG_DSP)

#------------------------------------------------------------------------------
# Nested variables
SRC_DEPS                    = $(foreach dep, $(BUILD_DEPENDENCIES), $(if $(findstring src,$($(dep)_TYPE)),$(dep)))
PLUG_DEPS                   = $(foreach dep, $(BUILD_DEPENDENCIES), $(if $(findstring plug,$($(dep)_TYPE)),$(dep)))
BIN_DEPS                    = $(foreach dep, $(BUILD_DEPENDENCIES), $(if $(findstring bin,$($(dep)_TYPE)),$(dep)))

#------------------------------------------------------------------------------
# Functional variables
CXX_FILE                    = $(patsubst $(LSP_PLUGIN_FW_BIN)/%.o,%.cpp, $(@))

## Format-related paths
#FMT_JACK_PATH               = $(ARTIFACT_BIN)/jack
#
## Binaries
#ARTIFACT_LIB_JACK           = $(ARTIFACT_BIN)/lib$(ARTIFACT_NAME)-jack-$(ARTIFACT_VERSION)$(LIBRARY_EXT)
#ARTIFACT_LIB_JACK_DEPS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(UI_DEPENDENCIES_JACK)), $(if $($(dep)_OBJ), $(dep)))
#ARTIFACT_LIB_JACK_LIBS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(UI_DEPENDENCIES_JACK)), $($(dep)_OBJ))
#ARTIFACT_LIB_JACK_LDFLAGS   = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(UI_DEPENDENCIES_JACK)), $($(dep)_LDFLAGS))
#ARTIFACT_LIB_JACK_OBJS      = $(ARTIFACT_OBJ_CORE) $(ARTIFACT_OBJ_META) $(ARTIFACT_OBJ_DSP) $(ARTIFACT_OBJ_UI) \
#                              $(OBJ_PLUG_META) $(OBJ_PLUG_DSP) $(OBJ_PLUG_UI)
#
#OBJ_PLUG_META               = $(foreach dep, $(DEPENDENCIES), $($(dep)_OBJ_META))
#OBJ_PLUG_DSP                = $(foreach dep, $(DEPENDENCIES), $($(dep)_OBJ_DSP))
#OBJ_PLUG_UI                 = $(foreach dep, $(DEPENDENCIES), $($(dep)_OBJ_UI))
#OBJ_PLUG_TEST               = $(foreach dep, $(DEPENDENCIES), $($(dep)_OBJ_TEST))
#OBJ_PLUG                    = $(OBJ_PLUG_META) $(OBJ_PLUG_DSP) $(OBJ_PLUG_UI)
#
#CFLAGS_DEPS                 = $(foreach dep, $(DEPENDENCIES) $(ARTIFACT_VARS), $(if $($(dep)_CFLAGS), $($(dep)_CFLAGS)))
#BUILD_DEPS                  = $(foreach dep, $(DEPENDENCIES), $(if $($(dep)_OBJ), $(dep)))
#PLUGIN_DEPS                 = $(foreach dep, $(DEPENDENCIES), $(if $($(dep)_OBJ_META), $(dep)))
#
#ifeq ($(TEST), 1)
#  CXX_SRC                    += $(CXX_SRC_TEST)
#  ARTIFACT_OBJ               += $(ARTIFACT_OBJ_TEST)
#  OBJ                        += $(OBJ_TEST)
#  DEPENDENCIES               += $(TEST_DEPENDENCIES)
#endif
#
#CXX_FILE                    = $(patsubst $(ARTIFACT_BIN)/%.o,%.cpp, $(@))
#CXX_DEP_ALL                 = $(foreach src, $(CXX_SRC),$(patsubst %.cpp,$(ARTIFACT_BIN)/%.d,$(src)))
#CXX_DEP_FILE                = $(patsubst $(ARTIFACT_BIN)/%.d,%.cpp,$(@))
#CXX_DEP_TARGET              = $(patsubst $(ARTIFACT_BIN)/%.d,%.o,$(@))
#
#ifeq ($(NO_DEPENDENCIES),1)
#  BUILD_DEPS                  =
#  PLUGIN_DEPS                 =
#endif
#
#$(info PLUGIN_DEPS = $(PLUGIN_DEPS))
#$(info OBJ_PLUG_META = $(OBJ_PLUG_META))
#$(info OBJ_PLUG_DSP = $(OBJ_PLUG_DSP))
#$(info OBJ_PLUG_UI = $(OBJ_PLUG_UI))
#$(info OBJ_PLUG_TEST = $(OBJ_PLUG_TEST))
#$(info UTL_JACK_MAKE_DEPS = $(UTL_JACK_MAKE_DEPS))
#$(info UTL_JACK_MAKE_OBJ = $(UTL_JACK_MAKE_OBJ))
#$(info UTL_JACK_MAKE_OBJS = $(UTL_JACK_MAKE_OBJS))
#$(info UTL_JACK_MAKE_LIBS = $(UTL_JACK_MAKE_LIBS))
#$(info $(UTL_JACK_MAKE): $(UTL_JACK_MAKE_OBJ) $(UTL_JACK_MAKE_DEPS) $(PLUGIN_DEPS))

$(info LSP_PLUGIN_FW_OBJ = $(LSP_PLUGIN_FW_OBJ))
$(info CXX_SRC_TEST = $(CXX_SRC_TEST))
$(info CXX_SRC = $(CXX_SRC))

.DEFAULT_GOAL = all
.PHONY: compile depend dep_clean all install uninstall
.PHONY: jack ladspa dssi lv2 vst
.PHONY: $(BUILD_DEPS)

# Dependencies
#dep_clean:
#
#$(CXX_DEP_ALL): dep_clean
#	@echo "  dep  [$(ARTIFACT_NAME)] $(CXX_DEP_FILE)"
#	@mkdir -p $(dir $(@))
#	$(CXX) -MM -MT "\$$(ARTIFACT_BIN)/$(CXX_DEP_TARGET)" -MF $(@) $(CXX_DEP_FILE) $(CXXFLAGS) $(INCLUDE) $(CFLAGS_DEPS)
#
#depend: $(CXX_DEP_ALL)
#	$(foreach dep,$(DEPENDENCIES) $(ARTIFACT_VARS),\
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
	@$(MAKE) -s -C "$($(@)_PATH)" compile CONFIG="$(CONFIG)" NO_DEPENDENCIES="1" DEMO_TEST="0"

$(BIN_DEPS):
	@echo "make [$($(@)_NAME)]"
	@$(MAKE) -s -C "$($(@)_PATH)" all CONFIG="$(CONFIG)" NO_DEPENDENCIES="1" DEMO_TEST="0"

# Compilation
$(OBJ):
	@echo "  $(CXX)  [$(LSP_PLUGIN_FW_OBJ)] $(CXX_FILE)"
	@mkdir -p $(dir $@)
	@$(CXX) -o $(@) -c $(CXX_FILE) -fPIC $(CXXFLAGS) $(LSP_PLUGIN_FW_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_CFLAGS)

#compile:
#
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

#
## Utilities
#$(UTL_JACK_MAKE): $(ARTIFACT_OBJ) $(UTL_JACK_MAKE_OBJ) $(UTL_JACK_MAKE_DEPS) $(PLUGIN_DEPS)
#	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(UTL_JACK_MAKE))"
#	@mkdir -p $(UTL_BIN_PATH)
#	@echo $(CXX) -o $(UTL_JACK_MAKE) $(UTL_JACK_MAKE_OBJ) $(UTL_JACK_LIBS) $(UTL_JACK_MAKE_OBJS) $(UTL_JACK_MAKE_LIBS) $(EXE_FLAGS) $(UTL_JACK_MAKE_LDFLAGS) 
#	@$(CXX) -o $(UTL_JACK_MAKE) $(UTL_JACK_MAKE_OBJ) $(UTL_JACK_LIBS) $(UTL_JACK_MAKE_OBJS) $(UTL_JACK_MAKE_LIBS) $(EXE_FLAGS) $(UTL_JACK_MAKE_LDFLAGS)
#
## Binaries
#$(ARTIFACT_LIB_JACK): $(ARTIFACT_OBJ) $(ARTIFACT_LIB_JACK_DEPS) $(PLUGIN_DEPS)
#	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_JACK))"
#	@echo $(CXX) -o $(ARTIFACT_LIB_JACK) $(ARTIFACT_LIB_JACK_LIBS) $(ARTIFACT_LIB_JACK_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_JACK_LDFLAGS)
#	@$(CXX) -o $(ARTIFACT_LIB_JACK) $(ARTIFACT_LIB_JACK_LIBS) $(ARTIFACT_LIB_JACK_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_JACK_LDFLAGS)
#
## All targets
#jack: $(UTL_JACK_MAKE)
#	@echo "  $(notdir $(UTL_JACK_MAKE)) [$(ARTIFACT_NAME)] $(patsubst $(ARTIFACT_BIN)/%,%, $(FMT_JACK_PATH))"
#	@mkdir -p "$(FMT_JACK_PATH)"
#	@$(UTL_JACK_MAKE) "$(FMT_JACK_PATH)"
#	@echo "  make JACK"
#	@make -C "$(FMT_JACK_PATH)" CONFIG="$(CONFIG)" LIBS="" \
#	         CXXFLAGS="$(CXXFLAGS) -DINSTALL_PREFIX=\"$(PREFIX)\"" \
#	         LDFLAGS="$(call query,LDFLAGS,$(DEPENDENCIES_SHARED))" \
#	         INCLUDE="$(INCLUDE) -I$($(ARTIFACT_VARS)_INC) $(call query,CFLAGS,$(DEPENDENCIES))"
#
all: $(SRC_DEPS) $(BIN_DEPS) $(LSP_PLUGIN_FW_OBJ)
#$(UTL_JACK_MAKE) $(ARTIFACT_LIB_JACK) jack

#CXX_SRCMAIN             = $(call rwildcard, main, *.cpp)
#CXX_SRCEXT              =
#CXX_SRCALL              = $(call rwildcard, main, *.cpp)
#CXX_SRCTEST             = $(call rwildcard, test, *.cpp)
#CXX_SRC                 = $(CXX_SRCMAIN)
#
#CXX_OBJ                 = $(patsubst %.cpp, $($(ARTIFACT_VARS)_BIN)/%.o, $(CXX_SRC))
#CXX_OBJEXT              = $(patsubst %.cpp, $($(ARTIFACT_VARS)_BIN)/%.o, $(CXX_SRCEXT))
#
#ALL_HEADERS             = $(call rwildcard, $($(ARTIFACT_VARS)_INC), *.h)
#CXX_FILE                = $(patsubst $($(ARTIFACT_VARS)_BIN)/%.o,%.cpp, $(@))
#CXX_HDR_PATHS           = $(foreach hdr,$(ARTIFACT_HEADERS),$($(ARTIFACT_VARS)_INC)/$(hdr))
#CXX_HEADERS             = $(foreach path,$(CXX_HDR_PATHS),$(call rwildcard, $(path), *.h))
#CXX_INSTHEADERS         = $(patsubst $($(ARTIFACT_VARS)_INC)/%,$(DESTDIR)$(INCDIR)/%,$(CXX_HEADERS))
#
#ARTIFACT_BIN            = $($(ARTIFACT_VARS)_BIN)
#ARTIFACT_LIB            = $(ARTIFACT_BIN)/$(LIBRARY_PREFIX)$($(ARTIFACT_VARS)_NAME)-$($(ARTIFACT_VARS)_VERSION)$(LIBRARY_EXT)
#ARTIFACT_SLIB           = $(ARTIFACT_BIN)/$(LIBRARY_PREFIX)$($(ARTIFACT_VARS)_NAME)-$($(ARTIFACT_VARS)_VERSION)$(STATICLIB_EXT)
#ARTIFACT_LIBLINK        = $(LIBRARY_PREFIX)$($(ARTIFACT_VARS)_NAME)$(LIBRARY_EXT)
#ARTIFACT_SLIBLINK       = $(LIBRARY_PREFIX)$($(ARTIFACT_VARS)_NAME)$(STATICLIB_EXT)
#ARTIFACT_PC             = $(ARTIFACT_BIN)/$($(ARTIFACT_VARS)_NAME)$(PKGCONFIG_EXT)
#ARTIFACT_TESTBIN        = $(ARTIFACT_BIN)/$($(ARTIFACT_VARS)_NAME)-test$(EXECUTABLE_EXT)
#ARTIFACT_OBJ            = $($(ARTIFACT_VARS)_OBJ)
#ARTIFACT_MFLAGS         = $($(ARTIFACT_VARS)_MFLAGS) $(foreach dep,$(DEPENDENCIES),-DUSE_$(dep))
#
#CFLAGS_DEPS             = $(foreach dep, $(DEPENDENCIES) $(ARTIFACT_VARS), $(if $($(dep)_CFLAGS), $($(dep)_CFLAGS)))
#BUILD_DEPS              = $(foreach dep, $(DEPENDENCIES), $(if $($(dep)_OBJ), $(dep)))
#LINKER_DEPS             = $(foreach dep, $(DEPENDENCIES), $(if $($(dep)_LDFLAGS), $($(dep)_LDFLAGS)))
#LINKER_OBJS             = $(foreach dep, $(DEPENDENCIES), $(if $($(dep)_OBJ), $($(dep)_OBJ)))
#BUILD_ALL               = $(ARTIFACT_LIB) $(ARTIFACT_SLIB) $(ARTIFACT_PC)
#
#ifeq ($(TEST), 1)
#  ifeq ($($(ARTIFACT_VARS)_TESTING),1)
#    CXX_SRC                += $(CXX_SRCTEST)
#    BUILD_ALL              += $(ARTIFACT_TESTBIN)
#  endif
#  DEPENDENCIES           += $(TEST_DEPENDENCIES)
#endif
#
#ifeq ($(NO_DEPENDENCIES),1)
#  BUILD_DEPS              =
#endif
#
#CXX_DEPS                = $(foreach src,$(CXX_SRCALL) $(CXX_SRCTEST),$(patsubst %.cpp,$(ARTIFACT_BIN)/%.d,$(src)))
#CXX_DEPFILE             = $(patsubst $(ARTIFACT_BIN)/%.d,%.cpp,$(@))
#CXX_DEPTARGET           = $(patsubst $(ARTIFACT_BIN)/%.d,%.o,$(@))
#
#compile: $(ARTIFACT_OBJ)
#
#all: $(BUILD_ALL)
#
#dep_clean:
#
#$(CXX_DEPS): dep_clean
#	@echo "  dep  [$(ARTIFACT_NAME)] $(CXX_DEPFILE)"
#	@mkdir -p $(dir $(@))
#	$(CXX) -MM -MT "\$$($(ARTIFACT_VARS)_BIN)/$(CXX_DEPTARGET)" -MF $(@) $(CXX_DEPFILE) $(CXXFLAGS) $(INCLUDE) $(CFLAGS_DEPS)
#
#depend: $(CXX_DEPS)
#	$(foreach dep,$(DEPENDENCIES) $(ARTIFACT_VARS),\
#	  $(if $($(dep)_INC), \
#	    $(eval SED_RULES += s/$(shell echo "$($(dep)_INC)" | sed "s/\\//\\\\\//g")/\$$$$\\($(dep)_INC\\)/g;)\
#	  )\
#	  $(if $($(dep)_BIN), \
#	    $(eval SED_RULES += s/$(shell echo "$($(dep)_BIN)" | sed "s/\\//\\\\\//g")/\\$$$$\\($(dep)_BIN\\)/g;)\
#	  )\
#	)
#	@cat $(CXX_DEPS) | sed -E "$(SED_RULES)" >Makefile.d
#
#$(BUILD_DEPS):
#	@echo "make $(notdir $($(@)_OBJ))"
#	@$(MAKE) -s -C "$($(@)_PATH)" compile CONFIG="$(CONFIG)" NO_DEPENDENCIES="1" DEMO_TEST="0"
#
#$(ARTIFACT_OBJ): $(BUILD_DEPS) $(CXX_OBJ) $(CXX_OBJEXT)
#	@echo "  $(LD)   [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_OBJ))"
#	@$(LD) -o $(ARTIFACT_OBJ) -r $(CXX_OBJ) $(CXX_OBJEXT)
#
#$(ARTIFACT_LIB): $(ARTIFACT_OBJ)
#	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB))"
#	@$(CXX) -o $(ARTIFACT_LIB) $(LINKER_OBJS) $(ARTIFACT_OBJ) $(SO_FLAGS) $(LINKER_DEPS)
#	
#$(ARTIFACT_SLIB): $(ARTIFACT_OBJ)
#	@echo "  $(AR)   [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_SLIB))"
#	@$(AR) rcs $(ARTIFACT_SLIB) $(LINKER_OBJS) $(ARTIFACT_OBJ)
#
#$(ARTIFACT_TESTBIN): $(ARTIFACT_OBJ)
#	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_TESTBIN))"
#	@$(CXX) -o $(ARTIFACT_TESTBIN) $(LINKER_OBJS) $(ARTIFACT_OBJ) $(EXE_FLAGS) $(LINKER_DEPS)
#
#$(CXX_OBJ) $(CXX_OBJEXT):
#	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(CXX_FILE)"
#	@mkdir -p $(dir $@)
#	@$(CXX) -o $(@) -c $(CXX_FILE) -fPIC $(CXXFLAGS) $(ARTIFACT_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(CFLAGS_DEPS)
#
#$(ARTIFACT_PC):
#	@mkdir -p $(dir $(ARTIFACT_PC))
#	@echo "prefix=$(PREFIX)" > "$(ARTIFACT_PC)"
#	@echo "exec_prefix=$(BINDIR)" >> "$(ARTIFACT_PC)"
#	@echo "includedir=$(INCDIR)" >> "$(ARTIFACT_PC)"
#	@echo "libdir=$(LIBDIR)" >> "$(ARTIFACT_PC)"
#	@echo "" >> $(ARTIFACT_PC) >> "$(ARTIFACT_PC)"
#	@echo "Name: $($(ARTIFACT_VARS)_NAME)" >> "$(ARTIFACT_PC)"
#	@echo "Description: $($(ARTIFACT_VARS)_DESC)" >> "$(ARTIFACT_PC)"
#	@echo "Version: $($(ARTIFACT_VARS)_VERSION)" >> "$(ARTIFACT_PC)"
#	@echo "Cflags: -I\$${includedir}" >> "$(ARTIFACT_PC)"
#	@echo "Libs: -L\$${libdir} -l$(notdir $($(ARTIFACT_VARS)_NAME))" >> "$(ARTIFACT_PC)"
#
#install: all
#	@echo "Installing $($(ARTIFACT_VARS)_NAME)"
#	@mkdir -p "$(DESTDIR)$(INCDIR)"
#	@mkdir -p "$(DESTDIR)$(LIBDIR)/pkgconfig"
#	@cp -r "$(CXX_HDR_PATHS)" "$(DESTDIR)$(INCDIR)/"
#	@cp $(ARTIFACT_PC) "$(DESTDIR)$(LIBDIR)/pkgconfig/"
#	@echo $(INSTALL) $(ARTIFACT_LIB) -t "$(DESTDIR)$(LIBDIR)"
#	@$(INSTALL) $(ARTIFACT_LIB) -t "$(DESTDIR)$(LIBDIR)"
#	@echo cp $(ARTIFACT_SLIB) -t "$(DESTDIR)$(LIBDIR)"
#	@cp $(ARTIFACT_SLIB) -t "$(DESTDIR)$(LIBDIR)"
#	@ln -sf $(notdir $(ARTIFACT_LIB)) "$(DESTDIR)$(LIBDIR)/$(ARTIFACT_LIBLINK)"
#	@ln -sf $(notdir $(ARTIFACT_SLIB)) "$(DESTDIR)$(LIBDIR)/$(ARTIFACT_SLIBLINK)"
#	@echo "Install OK"
#
#uninstall:
#	@echo "Uninstalling $($(ARTIFACT_VARS)_NAME)"
#	@-rm -f "$(DESTDIR)$(LIBDIR)/$(notdir $(ARTIFACT_LIB))"
#	@-rm -f "$(DESTDIR)$(LIBDIR)/$(notdir $(ARTIFACT_SLIB))"
#	@-rm -f "$(DESTDIR)$(LIBDIR)/$(ARTIFACT_LIBLINK)"
#	@-rm -f "$(DESTDIR)$(LIBDIR)/$(ARTIFACT_SLIBLINK)"
#	@-rm -f "$(DESTDIR)$(LIBDIR)/pkgconfig/$(notdir $(ARTIFACT_PC))"
#	@-rm -f $(CXX_INSTHEADERS)
#	@echo "Uninstall OK"
#


# Dependencies
-include $(CURDIR)/Makefile.d


