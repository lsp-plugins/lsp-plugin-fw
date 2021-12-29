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

# Command-line flag to silence nested $(MAKE).
ifneq ($(VERBOSE),1)
.SILENT:
endif

BASEDIR                 = $(CURDIR)
CONFIG                 := $(CURDIR)/.config.mk
PROJECT                := $(CURDIR)/../project.mk
PLUGINS                := $(CURDIR)/../plugins.mk

include $(CONFIG)
include $(PLUGINS)
include $(PROJECT)
include $(BASEDIR)/../make/functions.mk

# Artifact settings
ifeq ($(BUILD_HOST),1)
  override HOST                    = HOST_
else
  override HOST                    =
endif

HOST_DEPENDENCIES       = $(DEPENDENCIES)
HOST_DEPENDENCIES_BIN   = $(DEPENDENCIES_BIN)
HOST_TEST_DEPENDENCIES  = $(TEST_DEPENDENCIES)
HOST_PLUGIN_DEPENDENCIES= $(PLUGIN_DEPENDENCIES)

ifeq ($(TEST),1)
  UNIQ_DEPENDENCIES      := $(call uniq, $(DEPENDENCIES) $(TEST_DEPENDENCIES))
  HOST_UNIQ_DEPENDENCIES := $(call uniq, $(HOST_DEPENDENCIES) $(HOST_TEST_DEPENDENCIES))
else
  UNIQ_DEPENDENCIES      := $(call uniq, $(DEPENDENCIES))
  HOST_UNIQ_DEPENDENCIES := $(call uniq, $(HOST_DEPENDENCIES))
endif

DEPENDENCIES            = $(filter-out $(DEPENDENCIES_BIN), $(UNIQ_DEPENDENCIES))
HOST_DEPENDENCIES_BIN   = $(DEPENDENCIES_BIN)
HOST_DEPENDENCIES       = $(filter-out $(HOST_DEPENDENCIES_BIN), $(HOST_UNIQ_DEPENDENCIES))

PLUGIN_ARTIFACT_PREFIX := $(if $(PLUGIN_ARTIFACT_ID),$(PLUGIN_ARTIFACT_ID)-,)
INSTALL_LIBDIR         := $(if $(PLUGIN_ARTIFACT_ID),$(PLUGIN_ARTIFACT_ID),$($(ARTIFACT_ID)_NAME))
JACK_LIBDIR            := $(LIBDIR)/$(INSTALL_LIBDIR)
LADSPA_LIBDIR          := $(LIBDIR)/ladspa
LV2_LIBDIR             := $(LIBDIR)/lv2/$(INSTALL_LIBDIR).lv2
VST2_LIBDIR            := $(LIBDIR)/vst/$(INSTALL_LIBDIR)

#------------------------------------------------------------------------------
# Target directories
ARTIFACT_BIN                = $($(ARTIFACT_ID)_BIN)
HOST_ARTIFACT_BIN           = $($(HOST)$(ARTIFACT_ID)_BIN)

#------------------------------------------------------------------------------
# Framework settings for (cross) build
LSP_PLUGIN_FW_SRC_RES               = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-res.cpp

LSP_PLUGIN_FW_OBJ_CORE              = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-core.o
LSP_PLUGIN_FW_OBJ_META              = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-meta.o
LSP_PLUGIN_FW_OBJ_DSP               = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-dsp.o
LSP_PLUGIN_FW_OBJ_UI                = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui.o
LSP_PLUGIN_FW_OBJ_CTL               = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ctl.o
LSP_PLUGIN_FW_OBJ_WRAP_CAIRO        = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-wrap-cairo.o
LSP_PLUGIN_FW_OBJ_TEST              = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-test.o
LSP_PLUGIN_FW_OBJ_RES               = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-res.o
LSP_PLUGIN_FW_OBJ                   = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_UI) \
  $(LSP_PLUGIN_FW_OBJ_CTL)

LSP_PLUGIN_FW_MFLAGS           += $(foreach dep,$(DEPENDENCIES),-DUSE_$(dep)) 
LSP_PLUGIN_FW_CFLAGS           += $(foreach dep,$(DEPENDENCIES), $($(dep)_CFLAGS))

# Framework settings for host build
HOST_LSP_PLUGIN_FW_SRC_RES          = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-res.cpp

HOST_LSP_PLUGIN_FW_OBJ_CORE         = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-core.o
HOST_LSP_PLUGIN_FW_OBJ_META         = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-meta.o
HOST_LSP_PLUGIN_FW_OBJ_DSP          = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-dsp.o
HOST_LSP_PLUGIN_FW_OBJ_UI           = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui.o
HOST_LSP_PLUGIN_FW_OBJ_CTL          = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ctl.o
HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO   = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-wrap-cairo.o
HOST_LSP_PLUGIN_FW_OBJ_TEST         = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-test.o
HOST_LSP_PLUGIN_FW_OBJ_RES          = $($(HOST)LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-res.o
HOST_LSP_PLUGIN_FW_OBJ              = \
  $(HOST_LSP_PLUGIN_FW_OBJ_CORE) \
  $(HOST_LSP_PLUGIN_FW_OBJ_META) \
  $(HOST_LSP_PLUGIN_FW_OBJ_DSP) \
  $(HOST_LSP_PLUGIN_FW_OBJ_UI) \
  $(HOST_LSP_PLUGIN_FW_OBJ_CTL)

HOST_LSP_PLUGIN_FW_MFLAGS  += $(foreach dep,$(DEPENDENCIES),-DUSE_$(dep)) 
HOST_LSP_PLUGIN_FW_CFLAGS  += $(foreach dep,$(DEPENDENCIES), $(HOST_$(dep)_CFLAGS))

# Source code location for (cross) build
CXX_SRC_CORE                = $(call rwildcard, main/core, *.cpp)
CXX_SRC_VERSION             = main/version.cpp
CXX_SRC_META                = $(call rwildcard, main/meta, *.cpp)
CXX_SRC_DSP                 = $(call rwildcard, main/plug, *.cpp)
CXX_SRC_UI                  = $(call rwildcard, main/ui, *.cpp)
CXX_SRC_CTL                 = $(call rwildcard, main/ctl, *.cpp)
CXX_SRC_WRAP_JACK           = wrap/jack.cpp
CXX_SRC_WRAP_LADSPA         = wrap/ladspa.cpp
CXX_SRC_WRAP_LV2            = wrap/lv2.cpp
CXX_SRC_WRAP_LV2_UI         = wrap/lv2ui.cpp
CXX_SRC_WRAP_VST2           = wrap/vst2.cpp
CXX_SRC_WRAP_CAIRO          = $(call rwildcard, wrap/cairo, *.cpp)
CXX_SRC_UTIL                = $(call rwildcard, util, *.cpp)
CXX_SRC_TEST                = $(call rwildcard, test, *.cpp)
CXX_SRC                     = \
  $(CXX_SRC_CORE) \
  $(CXX_SRC_VERSION) \
  $(CXX_SRC_META) \
  $(CXX_SRC_DSP) \
  $(CXX_SRC_UI) \
  $(CXX_SRC_CTL) \
  $(CXX_SRC_WRAP_JACK) \
  $(CXX_SRC_WRAP_LADSPA) \
  $(CXX_SRC_WRAP_LV2) \
  $(CXX_SRC_WRAP_LV2_UI) \
  $(CXX_SRC_WRAP_VST2) \
  $(CXX_SRC_WRAP_CAIRO) \
  $(CXX_SRC_UTIL)

# Source code location for host
HOST_CXX_SRC_CORE           = $(CXX_SRC_CORE)
HOST_CXX_SRC_VERSION        = $(CXX_SRC_VERSION)
HOST_CXX_SRC_META           = $(CXX_SRC_META)
HOST_CXX_SRC_DSP            = $(CXX_SRC_DSP)
HOST_CXX_SRC_UI             = $(CXX_SRC_UI)
HOST_CXX_SRC_CTL            = $(CXX_SRC_CTL)
HOST_CXX_SRC_WRAP_JACK      = $(CXX_SRC_WRAP_JACK)
HOST_CXX_SRC_WRAP_LADSPA    = $(CXX_SRC_WRAP_LADSPA)
HOST_CXX_SRC_WRAP_LV2       = $(CXX_SRC_WRAP_LV2)
HOST_CXX_SRC_WRAP_LV2_UI    = $(CXX_SRC_WRAP_LV2_UI)
HOST_CXX_SRC_WRAP_VST2      = $(CXX_SRC_WRAP_VST2)
HOST_CXX_SRC_WRAP_CAIRO     = $(CXX_SRC_WRAP_CAIRO)
HOST_CXX_SRC_UTIL           = $(CXX_SRC_UTIL)
HOST_CXX_SRC_TEST           = $(CXX_SRC_TEST)
HOST_CXX_SRC                = \
  $(HOST_CXX_SRC_CORE) \
  $(HOST_CXX_SRC_VERSION) \
  $(HOST_CXX_SRC_META) \
  $(HOST_CXX_SRC_DSP) \
  $(HOST_CXX_SRC_UI) \
  $(HOST_CXX_SRC_CTL) \
  $(HOST_CXX_SRC_WRAP_JACK) \
  $(HOST_CXX_SRC_WRAP_LADSPA) \
  $(HOST_CXX_SRC_WRAP_LV2) \
  $(HOST_CXX_SRC_WRAP_LV2_UI) \
  $(HOST_CXX_SRC_WRAP_VST2) \
  $(HOST_CXX_SRC_WRAP_CAIRO) \
  $(HOST_CXX_SRC_UTIL)

# Object files for (cross) build
OBJ_STUB                    = $(patsubst %.cpp, %.o, $(CXX_SRC_STUB))
OBJ_CORE                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_CORE))
OBJ_VERSION                 = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_VERSION))
OBJ_META                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_META))
OBJ_DSP                     = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_DSP))
OBJ_UI                      = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_UI))
OBJ_CTL                     = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_CTL))
OBJ_WRAP_JACK               = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_JACK))
OBJ_WRAP_LADSPA             = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_LADSPA))
OBJ_WRAP_LV2                = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_LV2))
OBJ_WRAP_LV2_UI             = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_LV2_UI))
OBJ_WRAP_VST2               = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_VST2))
OBJ_WRAP_CAIRO              = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_CAIRO))
OBJ_UTIL                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_UTIL))
OBJ_TEST                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_TEST))
OBJ                         = \
  $(OBJ_CORE) \
  $(OBJ_VERSION) \
  $(OBJ_META) \
  $(OBJ_DSP) \
  $(OBJ_UI) \
  $(OBJ_CTL) \
  $(OBJ_UTIL) \
  $(OBJ_WRAP_JACK) \
  $(OBJ_WRAP_LADSPA) \
  $(OBJ_WRAP_LV2) \
  $(OBJ_WRAP_LV2_UI) \
  $(OBJ_WRAP_VST2) \
  $(OBJ_WRAP_CAIRO)

ifeq ($(TEST),1)
  CXX_SRC                    += $(CXX_SRC_TEST)
  HOST_CXX_SRC               += $(HOST_CXX_SRC_TEST)
  OBJ                        += $(OBJ_TEST)
  LSP_PLUGIN_FW_OBJ          += $(LSP_PLUGIN_FW_OBJ_TEST)
endif

# Object files for host build
HOST_OBJ_STUB               = $(patsubst %.cpp, %.o, $(HOST_CXX_SRC_STUB))
HOST_OBJ_CORE               = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_CORE))
HOST_OBJ_VERSION            = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_VERSION))
HOST_OBJ_META               = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_META))
HOST_OBJ_DSP                = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_DSP))
HOST_OBJ_UI                 = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_UI))
HOST_OBJ_CTL                = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_CTL))
HOST_OBJ_WRAP_JACK          = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_JACK))
HOST_OBJ_WRAP_LADSPA        = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_LADSPA))
HOST_OBJ_WRAP_LV2           = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_LV2))
HOST_OBJ_WRAP_LV2_UI        = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_LV2_UI))
HOST_OBJ_WRAP_VST2          = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_VST2))
HOST_OBJ_WRAP_CAIRO         = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_CAIRO))
HOST_OBJ_UTIL               = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_UTIL))
HOST_OBJ_TEST               = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_TEST))
HOST_OBJ                    = \
  $(HOST_OBJ_CORE) \
  $(HOST_OBJ_VERSION) \
  $(HOST_OBJ_META) \
  $(HOST_OBJ_DSP) \
  $(HOST_OBJ_UI) \
  $(HOST_OBJ_CTL) \
  $(HOST_OBJ_UTIL) \
  $(HOST_OBJ_WRAP_JACK) \
  $(HOST_OBJ_WRAP_LADSPA) \
  $(HOST_OBJ_WRAP_LV2) \
  $(HOST_OBJ_WRAP_LV2_UI) \
  $(HOST_OBJ_WRAP_VST2) \
  $(HOST_OBJ_WRAP_CAIRO)

ifeq ($(TEST),1)
  HOST_CXX_SRC               += $(HOST_CXX_SRC_TEST)
  HOST_OBJ                   += $(HOST_OBJ_TEST)
  HOST_LSP_PLUGIN_FW_OBJ     += $(HOST_LSP_PLUGIN_FW_OBJ_TEST)
endif

#------------------------------------------------------------------------------
# Utilities
UTL_BIN_PATH                = $(HOST_ARTIFACT_BIN)/utils-bin
UTL_RES_PATH                = $(ARTIFACT_BIN)/res
UTL_RES_CHECKSUM            = $(ARTIFACT_BIN)/res-checksum.json

UTL_COMMON_OBJ              = $(patsubst %.cpp,$(HOST_ARTIFACT_BIN)/%.o,$(call rwildcard, util/common, *.cpp))

UTL_JACK_MAKE               = $(UTL_BIN_PATH)/jack_make$(EXECUTABLE_EXT)
UTL_JACK_MAKE_OBJ           = $(patsubst %.cpp,$(HOST_ARTIFACT_BIN)/%.o,$(call rwildcard, util/jack_make, *.cpp))
UTL_JACK_MAKE_MAIN_OBJ      = $(HOST_ARTIFACT_BIN)/util/jack_make.o
UTL_JACK_MAKE_DEPS          = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $(if $($(dep)_OBJ), $(dep)))
UTL_JACK_MAKE_LIBS          = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_OBJ))
UTL_JACK_MAKE_LDFLAGS       = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_LDFLAGS))
UTL_JACK_MAKE_OBJS          = \
  $(HOST_LSP_PLUGIN_FW_OBJ_CORE) \
  $(HOST_LSP_PLUGIN_FW_OBJ_META) \
  $(HOST_LSP_PLUGIN_FW_OBJ_DSP) \
  $(HOST_LSP_PLUGIN_FW_OBJ_RES) \
  $(HOST_OBJ_PLUG_META) \
  $(HOST_OBJ_PLUG_DSP) \
  $(UTL_JACK_MAKE_OBJ) \
  $(UTL_COMMON_OBJ) \
  $(UTL_JACK_MAKE_MAIN_OBJ)

UTL_VST2_MAKE               = $(UTL_BIN_PATH)/vst2_make$(EXECUTABLE_EXT)
UTL_VST2_MAKE_OBJ           = $(patsubst %.cpp,$(HOST_ARTIFACT_BIN)/%.o,$(call rwildcard, util/vst2_make, *.cpp))
UTL_VST2_MAKE_MAIN_OBJ      = $(HOST_ARTIFACT_BIN)/util/vst2_make.o
UTL_VST2_MAKE_DEPS          = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $(if $($(dep)_OBJ), $(dep)))
UTL_VST2_MAKE_LIBS          = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_OBJ))
UTL_VST2_MAKE_LDFLAGS       = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_LDFLAGS))
UTL_VST2_MAKE_OBJS          = \
  $(HOST_LSP_PLUGIN_FW_OBJ_CORE) \
  $(HOST_LSP_PLUGIN_FW_OBJ_META) \
  $(HOST_LSP_PLUGIN_FW_OBJ_DSP) \
  $(HOST_LSP_PLUGIN_FW_OBJ_RES) \
  $(HOST_OBJ_PLUG_META) \
  $(HOST_OBJ_PLUG_DSP) \
  $(UTL_VST2_MAKE_OBJ) \
  $(UTL_COMMON_OBJ) \
  $(UTL_VST2_MAKE_MAIN_OBJ)

UTL_LV2TTL_GEN              = $(UTL_BIN_PATH)/lv2ttl_gen$(EXECUTABLE_EXT)
UTL_LV2TTL_GEN_OBJ          = $(patsubst %.cpp,$(HOST_ARTIFACT_BIN)/%.o,$(call rwildcard, util/lv2ttl_gen, *.cpp))
UTL_LV2TTL_GEN_MAIN_OBJ     = $(HOST_ARTIFACT_BIN)/util/lv2ttl_gen.o
UTL_LV2TTL_GEN_DEPS         = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $(if $($(dep)_OBJ), $(dep)))
UTL_LV2TTL_GEN_LIBS         = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_OBJ))
UTL_LV2TTL_GEN_LDFLAGS      = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_LDFLAGS))
UTL_LV2TTL_GEN_OBJS         = \
  $(HOST_LSP_PLUGIN_FW_OBJ_CORE) \
  $(HOST_LSP_PLUGIN_FW_OBJ_META) \
  $(HOST_LSP_PLUGIN_FW_OBJ_DSP) \
  $(HOST_LSP_PLUGIN_FW_OBJ_RES) \
  $(HOST_OBJ_PLUG_META) \
  $(HOST_OBJ_PLUG_DSP) \
  $(UTL_LV2TTL_GEN_OBJ) \
  $(UTL_LV2TTL_GEN_MAIN_OBJ)

UTL_REPOSITORY              = $(UTL_BIN_PATH)/repository$(EXECUTABLE_EXT)
UTL_REPOSITORY_OBJ          = $(patsubst %.cpp,$(HOST_ARTIFACT_BIN)/%.o,$(call rwildcard, util/repository, *.cpp))
UTL_REPOSITORY_MAIN_OBJ     = $(HOST_ARTIFACT_BIN)/util/repository.o
UTL_REPOSITORY_DEPS         = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $(if $($(dep)_OBJ), $(dep)))
UTL_REPOSITORY_LIBS         = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_OBJ))
UTL_REPOSITORY_LDFLAGS      = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_LDFLAGS))
UTL_REPOSITORY_OBJS         = \
  $(HOST_LSP_PLUGIN_FW_OBJ_CORE) \
  $(UTL_COMMON_OBJ) \
  $(UTL_REPOSITORY_OBJ) \
  $(UTL_REPOSITORY_MAIN_OBJ)

UTL_RESPACK                 = $(UTL_BIN_PATH)/respack$(EXECUTABLE_EXT)
UTL_RESPACK_OBJ             = $(patsubst %.cpp,$(HOST_ARTIFACT_BIN)/%.o,$(call rwildcard, util/respack, *.cpp))
UTL_RESPACK_MAIN_OBJ        = $(HOST_ARTIFACT_BIN)/util/respack.o
UTL_RESPACK_DEPS            = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $(if $($(dep)_OBJ), $(dep)))
UTL_RESPACK_LIBS            = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_OBJ))
UTL_RESPACK_LDFLAGS         = $(foreach dep, $(call uniq, $(HOST_DEPENDENCIES)), $($(dep)_LDFLAGS))
UTL_RESPACK_OBJS            = \
  $(HOST_LSP_PLUGIN_FW_OBJ_CORE) \
  $(UTL_COMMON_OBJ) \
  $(UTL_RESPACK_OBJ) \
  $(UTL_RESPACK_MAIN_OBJ)

#------------------------------------------------------------------------------
# Plugins
OBJ_PLUG_META               = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_META))
OBJ_PLUG_DSP                = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_DSP))
OBJ_PLUG_UI                 = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_UI))
OBJ_PLUG_TEST               = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_TEST))
OBJ_PLUG                    = $(OBJ_PLUG_META) $(OBJ_PLUG_DSP) $(OBJ_PLUG_UI)

HOST_OBJ_PLUG_META          = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_META))
HOST_OBJ_PLUG_DSP           = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_DSP))
HOST_OBJ_PLUG_UI            = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_UI))
HOST_OBJ_PLUG_TEST          = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_TEST))
HOST_OBJ_PLUG               = $(HOST_OBJ_PLUG_META) $(HOST_OBJ_PLUG_DSP) $(HOST_OBJ_PLUG_UI)

#------------------------------------------------------------------------------
# Binaries
ARTIFACT_LIB_JACK           = $(ARTIFACT_BIN)/lib$(ARTIFACT_NAME)-jack-$(ARTIFACT_VERSION)$(LIBRARY_EXT)
ARTIFACT_LIB_JACK_PATH      = $(ARTIFACT_BIN)/jack
ARTIFACT_LIB_JACK_SETTINGS  = $(ARTIFACT_LIB_JACK_PATH)/.settings.mk
ARTIFACT_LIB_JACK_DEPS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI)), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_JACK_LIBS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI)), $($(dep)_OBJ))
ARTIFACT_LIB_JACK_LDFLAGS   = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI)), $($(dep)_LDFLAGS))
ARTIFACT_LIB_JACK_OBJS      = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_VERSION) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_UI) \
  $(LSP_PLUGIN_FW_OBJ_RES) \
  $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_UI) \
  $(OBJ_WRAP_JACK)
  
ARTIFACT_LIB_LADSPA         = $(ARTIFACT_BIN)/$(ARTIFACT_NAME)-ladspa-$(ARTIFACT_VERSION)$(LIBRARY_EXT)
ARTIFACT_LIB_LADSPA_DEPS    = $(foreach dep, $(call uniq, $(DEPENDENCIES_LADSPA)), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_LADSPA_LIBS    = $(foreach dep, $(call uniq, $(DEPENDENCIES_LADSPA)), $($(dep)_OBJ))
ARTIFACT_LIB_LADSPA_LDFLAGS = $(foreach dep, $(call uniq, $(DEPENDENCIES_LADSPA)), $($(dep)_LDFLAGS))
ARTIFACT_LIB_LADSPA_OBJS    = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_RES) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_WRAP_LADSPA)
  
ARTIFACT_LIB_LV2            = $(ARTIFACT_BIN)/$(ARTIFACT_NAME)-lv2-$(ARTIFACT_VERSION)$(LIBRARY_EXT)
ARTIFACT_LIB_LV2_DEPS       = $(foreach dep, $(call uniq, $(DEPENDENCIES_LV2)), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_LV2_LIBS       = $(foreach dep, $(call uniq, $(DEPENDENCIES_LV2)), $($(dep)_OBJ))
ARTIFACT_LIB_LV2_LDFLAGS    = $(foreach dep, $(call uniq, $(DEPENDENCIES_LV2)), $($(dep)_LDFLAGS))
ARTIFACT_LIB_LV2_OBJS       = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_RES) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_WRAP_LV2)

ARTIFACT_LIB_LV2_UI         = $(ARTIFACT_BIN)/$(ARTIFACT_NAME)-lv2ui-$(ARTIFACT_VERSION)$(LIBRARY_EXT)
ARTIFACT_LIB_LV2_UI_DEPS    = $(foreach dep, $(call uniq, $(DEPENDENCIES_LV2_UI)), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_LV2_UI_LIBS    = $(foreach dep, $(call uniq, $(DEPENDENCIES_LV2_UI)), $($(dep)_OBJ))
ARTIFACT_LIB_LV2_UI_LDFLAGS = $(foreach dep, $(call uniq, $(DEPENDENCIES_LV2_UI)), $($(dep)_LDFLAGS))
ARTIFACT_LIB_LV2_UI_OBJS    = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_RES) \
  $(LSP_PLUGIN_FW_OBJ_UI) \
  $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_UI) \
  $(OBJ_WRAP_LV2_UI)
  
ARTIFACT_LIB_VST2           = $(ARTIFACT_BIN)/$(ARTIFACT_NAME)-vst2-$(ARTIFACT_VERSION)$(LIBRARY_EXT)
ARTIFACT_LIB_VST2_PATH      = $(ARTIFACT_BIN)/vst2
ARTIFACT_LIB_VST2_SETTINGS  = $(ARTIFACT_LIB_VST2_PATH)/.settings.mk
ARTIFACT_LIB_VST2_DEPS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_VST2)), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_VST2_LIBS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_VST2)), $($(dep)_OBJ))
ARTIFACT_LIB_VST2_LDFLAGS   = $(foreach dep, $(call uniq, $(DEPENDENCIES_VST2)), $($(dep)_LDFLAGS))
ARTIFACT_LIB_VST2_OBJS      = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_VERSION) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_UI) \
  $(LSP_PLUGIN_FW_OBJ_RES) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_UI) \
  $(OBJ_WRAP_VST2)

ifeq ($(TEST),1)
  ARTIFACT_BIN_TEST           = $(ARTIFACT_BIN)/$(ARTIFACT_NAME)-test$(EXECUTABLE_EXT)
  ARTIFACT_BIN_TEST_PATH      = $(ARTIFACT_BIN)
  ARTIFACT_BIN_TEST_DEPS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI) $(TEST_DEPENDENCIES)), $(if $($(dep)_OBJ), $(dep)))
  ARTIFACT_BIN_TEST_LIBS      = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI) $(TEST_DEPENDENCIES)), $($(dep)_OBJ))
  ARTIFACT_BIN_TEST_LDFLAGS   = $(foreach dep, $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI) $(TEST_DEPENDENCIES)), $($(dep)_LDFLAGS))
  ARTIFACT_BIN_TEST_OBJS      = \
    $(LSP_PLUGIN_FW_OBJ_CORE) \
    $(LSP_PLUGIN_FW_OBJ_META) \
    $(LSP_PLUGIN_FW_OBJ_DSP) \
    $(LSP_PLUGIN_FW_OBJ_UI) \
    $(LSP_PLUGIN_FW_OBJ_CTL) \
    $(LSP_PLUGIN_FW_OBJ_RES) \
    $(LSP_PLUGIN_FW_OBJ_TEST) \
    $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) \
    $(OBJ_PLUG_META) \
    $(OBJ_PLUG_DSP) \
    $(OBJ_PLUG_UI) \
    $(OBJ_WRAP_JACK) \
    $(OBJ_WRAP_LADSPA) \
    $(OBJ_WRAP_LV2) \
    $(OBJ_WRAP_LV2UI) \
    $(OBJ_WRAP_VST2) \
    $(UTL_COMMON_OBJ) \
    $(UTL_JACK_MAKE_OBJ) \
    $(UTL_VST2_MAKE_OBJ) \
    $(UTL_LV2TTL_GEN_OBJ) \
    $(UTL_REPOSITORY_OBJ) \
    $(UTL_RESPACK_OBJ)
endif

BIN_INSTALL                 = $(foreach dep, $(DEPENDENCIES_BIN), install_$(dep))
BIN_UNINSTALL               = $(foreach dep, $(DEPENDENCIES_BIN), uninstall_$(dep))
BIN_INST_ARTIFACT_ID        = $(patsubst install_%,%,$(@))
BIN_UNINST_ARTIFACT_ID      = $(patsubst uninstall_%,%,$(@))

#------------------------------------------------------------------------------
# Nested variables
SRC_DEPS                    = $(foreach dep, $(DEPENDENCIES), $(if $(findstring src,$($(dep)_TYPE)),$(dep)))
PLUG_DEPS                   = $(foreach dep, $(DEPENDENCIES) $(PLUGIN_DEPENDENCIES), $(if $(findstring plug,$($(dep)_TYPE)),$(dep)))

HOST_SRC_DEPS               = $(foreach dep, $(HOST_DEPENDENCIES), $(if $(findstring src,$($(dep)_TYPE)),$(dep)))
HOST_PLUG_DEPS              = $(foreach dep, $(HOST_DEPENDENCIES) $(HOST_PLUGIN_DEPENDENCIES), $(if $(findstring plug,$($(dep)_TYPE)),$(dep)))

#------------------------------------------------------------------------------
# Functional variables
CXX_FILE                    = $(patsubst $(LSP_PLUGIN_FW_BIN)/%.o,%.cpp, $(@))
HOST_CXX_FILE               = $(patsubst $(HOST_LSP_PLUGIN_FW_BIN)/%.o,%.cpp, $(@))

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
  -DPLUGIN_PACKAGE_BRANCH=\"$(PLUGIN_PACKAGE_BRANCH)\"
WRAP_JACK_LDFLAGS          := $(call query,LDFLAGS,$(DEPENDENCIES_JACK_WRAP))
WRAP_JACK_OBJS             := $(call query,OBJ,$(DEPENDENCIES_JACK_WRAP))
WRAP_JACK_INC              := $(call query,CFLAGS,$(DEPENDENCIES_JACK_WRAP) LSP_PLUGIN_FW)

WRAP_VST2_CFLAGS           := \
  -DINSTALL_PREFIX=\"$(PREFIX)\" \
  -DPLUGIN_PACKAGE_MAJOR=$(PLUGIN_PACKAGE_MAJOR) \
  -DPLUGIN_PACKAGE_MINOR=$(PLUGIN_PACKAGE_MINOR) \
  -DPLUGIN_PACKAGE_MICRO=$(PLUGIN_PACKAGE_MICRO) \
  -DPLUGIN_PACKAGE_BRANCH=\"$(PLUGIN_PACKAGE_BRANCH)\"
WRAP_VST2_LDFLAGS          := $(call query,LDFLAGS,$(DEPENDENCIES_VST2_WRAP))
WRAP_VST2_OBJS             := $(call query,OBJ,$(DEPENDENCIES_VST2_WRAP))
WRAP_VST2_INC              := $(call query,CFLAGS,$(DEPENDENCIES_VST2_WRAP) LSP_PLUGIN_FW)

#------------------------------------------------------------------------------
# Dependencies
DEP_CXX                 = $(foreach src,$(CXX_SRC),$(patsubst %.cpp,$(ARTIFACT_BIN)/%.d,$(src)))
DEP_CXX_FILE            = $(patsubst $(ARTIFACT_BIN)/%.d,%.cpp,$(@))
DEP_DEP_FILE            = $(patsubst $(ARTIFACT_BIN)/%.d,%.o,$(@))
DEP_CFLAGS              = $(call query,CFLAGS,$(ALL_DEPENDENCIES))

.DEFAULT_GOAL = all
.PHONY: compile depend dep_clean all install uninstall
.PHONY: jack ladspa dssi lv2 vst2 test
.PHONY: resources
.PHONY: install_jack install_ladspa install_lv2 install_vst2
.PHONY: uninstall_jack uninstall_ladspa uninstall_lv2 uninstall_vst2
.PHONY: $(DEPENDENCIES) $(DEPENDENCIES_BIN)
.PHONY: $(BIN_INSTALL) $(BIN_UNINSTALL)

#------------------------------------------------------------------------------
# Dependencies
dep_clean:

$(DEP_CXX): dep_clean
	echo "  dep  [$(ARTIFACT_NAME)] $(DEP_CXX_FILE)"
	mkdir -p $(dir $(@))
	$(CXX) -MM -MT "\$$($(ARTIFACT_ID)_BIN)/$(DEP_DEP_FILE)" -MF $(@) $(DEP_CXX_FILE) $(CXXFLAGS) $(INCLUDE) $(DEP_CFLAGS)

depend: $(DEP_CXX)
	$(foreach dep,$(ALL_DEPENDENCIES) $(ARTIFACT_ID),\
	  $(if $($(dep)_INC), \
	    $(eval SED_RULES += s/$(shell echo "$($(dep)_INC)" | sed "s/\\//\\\\\//g")/\$$$$\\($(dep)_INC\\)/g;)\
	  )\
	  $(if $($(dep)_BIN), \
	    $(eval SED_RULES += s/$(shell echo "$($(dep)_BIN)" | sed "s/\\//\\\\\//g")/\\$$$$\\($(dep)_BIN\\)/g;)\
	  )\
	)
	cat $(DEP_CXX) | sed -E "$(SED_RULES)" >Makefile.d

#------------------------------------------------------------------------------
# Derived source code libraries
$(SRC_DEPS) $(PLUG_DEPS):
	echo "make [$($(@)_NAME)]"
	mkdir -p "$($(@)_BIN)"
	$(MAKE) -C "$($(@)_PATH)" compile VERBOSE="$(VERBOSE)" PROJECT="$(PROJECT)" CONFIG="$(CONFIG)" ARTIFACT_ID="$(@)" NO_DEPENDENCIES="1" DEMO_TEST="0"

$(DEPENDENCIES_BIN):
	echo "make [$($(@)_NAME)]"
	$(MAKE) -C "$($(@)_PATH)" all VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" DEMO_TEST="0"

$(OBJ_PLUG_META) $(OBJ_PLUG_DSP) $(OBJ_PLUG_UI) $(OBJ_PLUG_TEST): $(PLUG_DEPS)

$(HOST_OBJ_PLUG_META) $(HOST_OBJ_PLUG_DSP) $(HOST_OBJ_PLUG_UI) $(HOST_OBJ_PLUG_TEST): $(PLUG_DEPS)

#------------------------------------------------------------------------------
# Compilation
$(OBJ):
	echo "  $(CXX)  [$(LSP_PLUGIN_FW_NAME)] $(CXX_FILE)"
	mkdir -p $(dir $@)
	$(CXX) -o $(@) -c $(CXX_FILE) -fPIC $(CXXFLAGS) $(LSP_PLUGIN_FW_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_CFLAGS)

$(OBJ_WRAP_JACK): EXT_FLAGS=$(WRAP_JACK_CFLAGS)

$(OBJ_WRAP_LADSPA): EXT_FLAGS=$(WRAP_LADSPA_CFLAGS)

$(OBJ_WRAP_LV2): EXT_FLAGS=$(WRAP_LV2_CFLAGS)

$(OBJ_WRAP_LV2_UI): EXT_FLAGS=$(WRAP_LV2_UI_CFLAGS)

$(OBJ_WRAP_VST2): EXT_FLAGS=$(WRAP_VST2_CFLAGS)

$(OBJ_WRAP_CAIRO): EXT_FLAGS=$(WRAP_CAIRO_CFLAGS)

ifeq ($(BUILD_HOST),1)

$(HOST_OBJ):
	echo "  $(HOST_CXX)  [$(LSP_PLUGIN_FW_NAME)] $(HOST_CXX_FILE)"
	mkdir -p $(dir $@)
	$(HOST_CXX) -o $(@) -c $(HOST_CXX_FILE) -fPIC $(HOST_CXXFLAGS) $(HOST_LSP_PLUGIN_FW_MFLAGS) $(HOST_EXT_FLAGS) $(INCLUDE) $(HOST_LSP_PLUGIN_FW_CFLAGS)

$(HOST_OBJ_WRAP_JACK): EXT_FLAGS=$(HOST_WRAP_JACK_CFLAGS)

$(HOST_OBJ_WRAP_LADSPA): EXT_FLAGS=$(HOST_WRAP_LADSPA_CFLAGS)

$(HOST_OBJ_WRAP_LV2): EXT_FLAGS=$(HOST_WRAP_LV2_CFLAGS)

$(HOST_OBJ_WRAP_LV2_UI): EXT_FLAGS=$(HOST_WRAP_LV2_UI_CFLAGS)

$(HOST_OBJ_WRAP_VST2): EXT_FLAGS=$(HOST_WRAP_VST2_CFLAGS)

$(HOST_OBJ_WRAP_CAIRO): EXT_FLAGS=$(HOST_WRAP_CAIRO_CFLAGS)

endif

#compile:

#------------------------------------------------------------------------------
# Linking targets
$(LSP_PLUGIN_FW_OBJ_CORE): $(OBJ_CORE)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_CORE))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_CORE) -r $(OBJ_CORE)
	
$(LSP_PLUGIN_FW_OBJ_META): $(OBJ_META)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_META))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_META) -r $(OBJ_META)

$(LSP_PLUGIN_FW_OBJ_DSP): $(OBJ_DSP)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_DSP))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_DSP) -r $(OBJ_DSP)
	
$(LSP_PLUGIN_FW_OBJ_UI): $(OBJ_UI)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_UI))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_UI) -r $(OBJ_UI)
	
$(LSP_PLUGIN_FW_OBJ_CTL): $(OBJ_CTL)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_CTL))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_CTL) -r $(OBJ_CTL)
	
$(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO): $(OBJ_WRAP_CAIRO)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) -r $(OBJ_WRAP_CAIRO)
	
$(LSP_PLUGIN_FW_OBJ_TEST): $(OBJ_TEST)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_TEST))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_TEST) -r $(OBJ_TEST)
	
$(LSP_PLUGIN_FW_OBJ_RES): $(UTL_RESPACK) resources
	echo "$(notdir $(UTL_RESPACK)) $(notdir $(LSP_PLUGIN_FW_SRC_RES))"
	echo $(UTL_RESPACK) -o "$(LSP_PLUGIN_FW_SRC_RES)" -i "$(UTL_RES_PATH)" -c "$(UTL_RES_CHECKSUM)"
	$(UTL_RESPACK) -o "$(LSP_PLUGIN_FW_SRC_RES)" -i "$(UTL_RES_PATH)" -c "$(UTL_RES_CHECKSUM)"
	echo $(CXX) -o $(@) -c $(LSP_PLUGIN_FW_SRC_RES) -fPIC $(CXXFLAGS) $(LSP_PLUGIN_FW_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_CFLAGS)
	$(CXX) -o $(@) -c $(LSP_PLUGIN_FW_SRC_RES) -fPIC $(CXXFLAGS) $(LSP_PLUGIN_FW_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_CFLAGS)

ifeq ($(BUILD_HOST),1)

$(HOST_LSP_PLUGIN_FW_OBJ_CORE): $(HOST_OBJ_CORE)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_CORE))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_CORE) -r $(HOST_OBJ_CORE)
	
$(HOST_LSP_PLUGIN_FW_OBJ_META): $(HOST_OBJ_META)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_META))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_META) -r $(HOST_OBJ_META)
	
$(HOST_LSP_PLUGIN_FW_OBJ_DSP): $(HOST_OBJ_DSP)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_DSP))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_DSP) -r $(HOST_OBJ_DSP)
	
$(HOST_LSP_PLUGIN_FW_OBJ_UI): $(HOST_OBJ_UI)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_UI))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_UI) -r $(HOST_OBJ_UI)
	
$(HOST_LSP_PLUGIN_FW_OBJ_CTL): $(HOST_OBJ_CTL)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_CTL))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_CTL) -r $(HOST_OBJ_CTL)
	
$(HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO): $(HOST_OBJ_WRAP_CAIRO)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) -r $(HOST_OBJ_WRAP_CAIRO)
	
$(HOST_LSP_PLUGIN_FW_OBJ_TEST): $(HOST_OBJ_TEST)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_TEST))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_TEST) -r $(HOST_OBJ_TEST)
	
$(HOST_LSP_PLUGIN_FW_OBJ_RES): $(UTL_RESPACK) resources
	echo "$(notdir $(UTL_RESPACK)) $(notdir $(HOST_LSP_PLUGIN_FW_SRC_RES))"
	echo $(UTL_RESPACK) -o "$(HOST_LSP_PLUGIN_FW_SRC_RES)" -i "$(UTL_RES_PATH)" -c "$(UTL_RES_CHECKSUM)"
	$(UTL_RESPACK) -o "$(HOST_LSP_PLUGIN_FW_SRC_RES)" -i "$(UTL_RES_PATH)" -c "$(UTL_RES_CHECKSUM)" 
	$(HOST_CXX) -o $(@) -c $(HOST_LSP_PLUGIN_FW_SRC_RES) -fPIC $(HOST_CXXFLAGS) $(HOST_LSP_PLUGIN_FW_MFLAGS) $(HOST_EXT_FLAGS) $(INCLUDE) $(HOST_LSP_PLUGIN_FW_CFLAGS)

endif

#------------------------------------------------------------------------------
# Utilities
$(UTL_JACK_MAKE): $(UTL_JACK_MAKE_DEPS) $(UTL_JACK_MAKE_OBJS) $(HOST_PLUG_DEPS)
	echo "  $(HOST_CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$(HOST_CXX) -o $(@) $(UTL_JACK_MAKE_OBJS) $(UTL_JACK_MAKE_LIBS) $(EXE_FLAGS) $(UTL_JACK_MAKE_LDFLAGS)
	
$(UTL_VST2_MAKE): $(UTL_VST2_MAKE_DEPS) $(UTL_VST2_MAKE_OBJS) $(HOST_PLUG_DEPS)
	echo "  $(HOST_CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$(HOST_CXX) -o $(@) $(UTL_VST2_MAKE_OBJS) $(UTL_VST2_MAKE_LIBS) $(EXE_FLAGS) $(UTL_VST2_MAKE_LDFLAGS)
	
$(UTL_LV2TTL_GEN): $(UTL_LV2TTL_GEN_DEPS) $(UTL_LV2TTL_GEN_OBJS) $(HOST_PLUG_DEPS)
	echo "  $(HOST_CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$(HOST_CXX) -o $(@) $(UTL_LV2TTL_GEN_OBJS) $(UTL_LV2TTL_GEN_LIBS) $(EXE_FLAGS) $(UTL_LV2TTL_GEN_LDFLAGS)

$(UTL_REPOSITORY): $(UTL_REPOSITORY_DEPS) $(UTL_REPOSITORY_OBJS) $(HOST_PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$(HOST_CXX) -o $(@) $(UTL_REPOSITORY_OBJS) $(UTL_REPOSITORY_LIBS) $(HOST_EXE_FLAGS) $(UTL_REPOSITORY_LDFLAGS)

$(UTL_RESPACK): $(UTL_RESPACK_DEPS) $(UTL_RESPACK_OBJS) $(HOST_PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$(HOST_CXX) -o $(@) $(UTL_RESPACK_OBJS) $(UTL_RESPACK_LIBS) $(HOST_EXE_FLAGS) $(UTL_RESPACK_LDFLAGS)

#------------------------------------------------------------------------------
# Binaries
$(ARTIFACT_LIB_JACK): $(LSP_PLUGIN_FW_OBJ) $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) $(OBJ_WRAP_JACK) $(ARTIFACT_LIB_JACK_DEPS) $(ARTIFACT_LIB_JACK_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_JACK))"
	$(CXX) -o $(ARTIFACT_LIB_JACK) $(ARTIFACT_LIB_JACK_LIBS) $(ARTIFACT_LIB_JACK_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_JACK_LDFLAGS)
	
$(ARTIFACT_LIB_LADSPA): $(LSP_PLUGIN_FW_OBJ) $(OBJ_WRAP_LADSPA) $(ARTIFACT_LIB_LADSPA_DEPS) $(ARTIFACT_LIB_LADSPA_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_LADSPA))"
	$(CXX) -o $(ARTIFACT_LIB_LADSPA) $(ARTIFACT_LIB_LADSPA_LIBS) $(ARTIFACT_LIB_LADSPA_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_LADSPA_LDFLAGS)
	
$(ARTIFACT_LIB_LV2): $(LSP_PLUGIN_FW_OBJ) $(OBJ_WRAP_LV2) $(ARTIFACT_LIB_LV2_DEPS) $(ARTIFACT_LIB_LV2_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_LV2))"
	$(CXX) -o $(ARTIFACT_LIB_LV2) $(ARTIFACT_LIB_LV2_LIBS) $(ARTIFACT_LIB_LV2_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_LV2_LDFLAGS)
	
$(ARTIFACT_LIB_LV2_UI): $(LSP_PLUGIN_FW_OBJ) $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) $(OBJ_WRAP_LV2_UI) $(ARTIFACT_LIB_LV2_UI_DEPS) $(ARTIFACT_LIB_LV2_UI_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_LV2_UI))"
	$(CXX) -o $(ARTIFACT_LIB_LV2_UI) $(ARTIFACT_LIB_LV2_UI_LIBS) $(ARTIFACT_LIB_LV2_UI_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_LV2_UI_LDFLAGS)
	
$(ARTIFACT_LIB_VST2): $(LSP_PLUGIN_FW_OBJ) $(OBJ_WRAP_VST2) $(ARTIFACT_LIB_VST2_DEPS) $(ARTIFACT_LIB_VST2_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_VST2))"
	$(CXX) -o $(ARTIFACT_LIB_VST2) $(ARTIFACT_LIB_VST2_LIBS) $(ARTIFACT_LIB_VST2_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_VST2_LDFLAGS)
	
$(ARTIFACT_BIN_TEST): $(LSP_PLUGIN_FW_OBJ) $(ARTIFACT_BIN_TEST_DEPS) $(ARTIFACT_BIN_TEST_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_BIN_TEST))"
	echo "ARTIFACT_BIN_TEST_OBJS = $(ARTIFACT_BIN_TEST_OBJS)"
	$(CXX) -o $(ARTIFACT_BIN_TEST) $(ARTIFACT_BIN_TEST_LIBS) $(ARTIFACT_BIN_TEST_OBJS) $(EXE_FLAGS) $(ARTIFACT_BIN_TEST_LDFLAGS)

# All targets
resources: $(UTL_REPOSITORY)
	echo "  $(UTL_REPOSITORY)"
	echo "$(UTL_REPOSITORY) $(UTL_RES_PATH)"
	echo $(UTL_REPOSITORY) -o "$(UTL_RES_PATH)" -l "$(ROOTDIR)/res/local" "$(ROOTDIR)/modules/*"
	$(UTL_REPOSITORY) -o "$(UTL_RES_PATH)" -l "$(ROOTDIR)/res/local" "$(ROOTDIR)/modules/*"

jack: $(ARTIFACT_LIB_JACK) $(UTL_JACK_MAKE)
	echo "  $(notdir $(UTL_JACK_MAKE)) [$(ARTIFACT_NAME)] $(patsubst $(ARTIFACT_BIN)/%,%, $(ARTIFACT_LIB_JACK_PATH))"
	mkdir -p "$(ARTIFACT_LIB_JACK_PATH)"
	$(UTL_JACK_MAKE) "$(ARTIFACT_LIB_JACK_PATH)"
	echo "  make JACK"
	echo -e \
	  '# Auto-generated script, do not edit\n\n' \
	  'EXT_ARTIFACT_ID=$(PLUGIN_ARTIFACT_ID)\n' \
	  'EXT_PREFIX=$(PLUGIN_ARTIFACT_PREFIX)\n' \
	  'EXT_OBJS=$(WRAP_JACK_OBJS)\n' \
	  'EXT_CFLAGS=$(WRAP_JACK_CFLAGS)\n' \
	  'EXT_CXXFLAGS=$(WRAP_JACK_CFLAGS)\n' \
	  'EXT_LDFLAGS=$(WRAP_JACK_LDFLAGS)\n' \
	  'EXT_INCLUDE=$(WRAP_JACK_INC)\n' > $(ARTIFACT_LIB_JACK_SETTINGS)
	$(MAKE) -C "$(ARTIFACT_LIB_JACK_PATH)" depend VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"
	$(MAKE) -C "$(ARTIFACT_LIB_JACK_PATH)"  VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"

ladspa: $(ARTIFACT_LIB_LADSPA) 

lv2: $(UTL_LV2TTL_GEN) $(ARTIFACT_LIB_LV2) $(ARTIFACT_LIB_LV2_UI)

vst2: $(ARTIFACT_LIB_VST2) $(UTL_VST2_MAKE)
	echo "  $(notdir $(UTL_VST2_MAKE)) [$(ARTIFACT_NAME)] $(patsubst $(ARTIFACT_BIN)/%,%, $(ARTIFACT_LIB_VST2_PATH))"
	mkdir -p "$(ARTIFACT_LIB_VST2_PATH)"
	$(UTL_VST2_MAKE) "$(ARTIFACT_LIB_VST2_PATH)"
	echo "  make VST2"
	echo -e \
	  '# Auto-generated script, do not edit\n\n' \
	  'EXT_ARTIFACT_ID=$(PLUGIN_ARTIFACT_ID)\n' \
	  'EXT_PREFIX=$(PLUGIN_ARTIFACT_PREFIX)\n' \
	  'EXT_OBJS=$(WRAP_VST2_OBJS)\n' \
	  'EXT_CFLAGS=$(WRAP_VST2_CFLAGS)\n' \
	  'EXT_CXXFLAGS=$(WRAP_VST2_CFLAGS)\n' \
	  'EXT_LDFLAGS=$(WRAP_VST2_LDFLAGS)\n' \
	  'EXT_INCLUDE=$(WRAP_VST2_INC)\n' > $(ARTIFACT_LIB_VST2_SETTINGS)
	$(MAKE) -C "$(ARTIFACT_LIB_VST2_PATH)" depend VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_VST2_SETTINGS)"
	$(MAKE) -C "$(ARTIFACT_LIB_VST2_PATH)"  VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_VST2_SETTINGS)"

test: $(ARTIFACT_BIN_TEST)

all: $(DEPENDENCIES_BIN) test jack ladspa lv2 vst2

#------------------------------------------------------------------------------
# Installation
$(BIN_INSTALL): $(DEPENDENCIES_BIN)
	echo "Install $(notdir $($(BIN_INST_ARTIFACT_ID)_PATH))"
	echo make -C "$($(BIN_INST_ARTIFACT_ID)_PATH)" install DESTDIR="$(DESTDIR)"
	make -C "$($(BIN_INST_ARTIFACT_ID)_PATH)" install DESTDIR="$(DESTDIR)"

install_jack: jack
	echo "Install jack"
	mkdir -p "$(DESTDIR)/$(JACK_LIBDIR)"
	mkdir -p "$(DESTDIR)/$(BINDIR)"
	$(INSTALL) $(ARTIFACT_LIB_JACK) "$(DESTDIR)$(JACK_LIBDIR)/"
	make -C "$(ARTIFACT_LIB_JACK_PATH)" install DESTDIR="$(DESTDIR)/$(BINDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"

install_ladspa: ladspa
	echo "Install ladspa"
	mkdir -p "$(DESTDIR)/$(LADSPA_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_LADSPA) "$(DESTDIR)/$(LADSPA_LIBDIR)/"

install_lv2: lv2
	echo "Install lv2"
	mkdir -p "$(DESTDIR)/$(LV2_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_LV2) "$(DESTDIR)/$(LV2_LIBDIR)/"
	$(INSTALL) $(ARTIFACT_LIB_LV2_UI) "$(DESTDIR)/$(LV2_LIBDIR)/"
	$(UTL_LV2TTL_GEN) -o "$(DESTDIR)/$(LV2_LIBDIR)" -i "$(ARTIFACT_LIB_LV2)" -ui $(ARTIFACT_LIB_LV2_UI)

install_vst2:
	echo "Install vst2"
	mkdir -p "$(DESTDIR)/$(VST2_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_VST2) "$(DESTDIR)/$(VST2_LIBDIR)/"
	make -C "$(ARTIFACT_LIB_VST2_PATH)" install DESTDIR="$(DESTDIR)/$(VST2_LIBDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_VST2_SETTINGS)"

install: $(BIN_INSTALL) install_jack install_ladspa install_lv2 install_vst2
	@echo "Install OK"

#------------------------------------------------------------------------------
# Deinstallation
$(BIN_UNINSTALL):
	echo "Uninstall $(notdir $($(BIN_UNINST_ARTIFACT_ID)_PATH))"
	make -C "$($(BIN_UNINST_ARTIFACT_ID)_PATH)" uninstall DESTDIR="$(DESTDIR)"

uninstall_jack:
	echo "Uninstall jack"
	-rm -rf "$(DESTDIR)$(JACK_LIBDIR)"
	make -C "$(ARTIFACT_LIB_JACK_PATH)" uninstall DESTDIR="$(DESTDIR)/$(BINDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"

uninstall_ladspa:
	echo "Uninstall ladspa"
	-rm -f "$(DESTDIR)/$(LADSPA_LIBDIR)/$(notdir $(ARTIFACT_LIB_LADSPA))"

uninstall_lv2:
	echo "Uninstall lv2"
	mkdir -p "$(DESTDIR)/$(LV2_LIBDIR)"
	-rm -rf "$(DESTDIR)/$(LV2_LIBDIR)"

uninstall_vst2:
	echo "Uninstall vst2"
	-rm -rf "$(DESTDIR)/$(VST2_LIBDIR)"

uninstall: $(BIN_UNINSTALL) uninstall_jack uninstall_ladspa uninstall_lv2 uninstall_vst2
	@echo "Uninstall OK"

# Dependencies
-include $(CURDIR)/Makefile.d


