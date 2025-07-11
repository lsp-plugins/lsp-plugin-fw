#
# Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
#
# This file is part of lsp-plugin-fw
#
# lsp-plugin-fw is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# lsp-plugin-fw is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with lsp-plugin-fw.  If not, see <https://www.gnu.org/licenses/>.
#

# Command-line flag to silence nested $(MAKE).
ifneq ($(VERBOSE),1)
.SILENT:
endif

MODDIR                  = $(CURDIR)/..
ROOTDIR                := $(MODDIR)
CONFIG                 := $(MODDIR)/.config.mk

# Project configuration
include $(CONFIG)
include $(ROOTDIR)/make/functions.mk

# Artifact dependencies
include $(MODDIR)/project.mk
include $(MODDIR)/dependencies.mk
MODDIR_DEPENDENCIES        := $(sort $(DEPENDENCIES))
MODDIR_TEST_DEPENDENCIES   := $(TEST_DEPENDENCIES)
MODDIR_ALL_DEPENDENCIES    := $(ALL_DEPENDENCIES)

# Injected plugin dependencies
include $(ROOTDIR)/dependencies.mk
include $(ROOTDIR)/plugins.mk
ROOTDIR_DEPENDENCIES       := $(sort $(DEPENDENCIES))
ROOTDIR_TEST_DEPENDENCIES  := $(TEST_DEPENDENCIES)
ROOTDIR_ALL_DEPENDENCIES   := $(ALL_DEPENDENCIES)

EXTRA_DEPENDENCIES         := $(subtract $(MODDIR_DEPENDENCIES),$(ROOTDIR_DEPENDENCIES))

# Artifact settings
ifeq ($(CROSS_COMPILE),1)
  override HOST                    = HOST_
else
  override HOST                    =
endif

ALLOWED_FEATURES            = clap doc gst jack ladspa lv2 ui vst2 vst3 xdg
ALLOWED_INSTALL_FEATURES    = clap doc gst jack ladspa lv2 vst2 vst3 xdg
INTERNAL_FEATURES           =
ifeq ($(TEST),1)
  INTERNAL_FEATURES            += test
endif
ENABLED_FEATURES           := $(call intersection,$(BUILD_FEATURES),$(ALLOWED_FEATURES))
ifdef FEATURES
  INSTALL_FEATURES           := $(call intersection,$(call intersection,$(FEATURES),$(ENABLED_FEATURES)),$(ALLOWED_INSTALL_FEATURES))
else
  INSTALL_FEATURES           := $(call intersection,$(ENABLED_FEATURES),$(ALLOWED_INSTALL_FEATURES))
endif
INTERNAL_FEATURES          += \
  $(call fcheck,$(ALLOWED_FEATURES),$(ENABLED_FEATURES),meta validate)

ENABLED_FEATURES_DEFS      := $(foreach feature,$(ENABLED_FEATURES),$(shell echo "$(feature)" | tr a-z A-Z))

# Setup test dependencies
ifeq ($(TEST),1)
  DEPENDENCIES_TEST      := $(sort $(MODDIR_TEST_DEPENDENCIES) $(ROOTDIR_TEST_DEPENDENCIES))
  DEPENDENCIES_TEST_UI   := $(sort $(TEST_DEPENDENCIES_UI))
else
  DEPENDENCIES_TEST      :=
  DEPENDENCIES_TEST_UI   :=
endif
HOST_DEPENDENCIES_TEST          = $(DEPENDENCIES_TEST)
HOST_DEPENDENCIES_TEST_UI       = $(DEPENDENCIES_TEST_UI)

MERGED_DEPENDENCIES             = \
  $(DEPENDENCIES_COMMON) \
  $(DEPENDENCIES_TEST) \
  $(call fcheck,test ui,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),\
    $(DEPENDENCIES_COMMON_UI) \
    $(DEPENDENCIES_TEST_UI) \
    $(DEPENDENCIES_BIN) \
  ) \
  $(call fcheck,clap,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_CLAP) \
    $(DEPENDENCIES_CLAP_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES), \
      $(DEPENDENCIES_CLAP_UI) \
      $(DEPENDENCIES_CLAP_UI_BIN)) \
  ) \
  $(call fcheck,gst,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_GST) \
    $(DEPENDENCIES_GST_BIN) \
  ) \
  $(call fcheck,jack test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $(DEPENDENCIES_JACK) \
    $(DEPENDENCIES_JACK_UI) \
    $(DEPENDENCIES_JACK_WRAP) \
    $(DEPENDENCIES_JACK_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_JACK_UI_BIN)) \
  ) \
  $(call fcheck,ladspa,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_LADSPA) \
    $(DEPENDENCIES_LADSPA_BIN) \
  ) \
  $(call fcheck,lv2,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_LV2) \
    $(DEPENDENCIES_LV2TTL_GEN) \
    $(DEPENDENCIES_LV2_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES), \
      $(DEPENDENCIES_LV2_UI) \
      $(DEPENDENCIES_LV2_UI_BIN)) \
  ) \
  $(call fcheck,vst2,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_VST2) \
    $(DEPENDENCIES_VST2_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES), \
      $(DEPENDENCIES_VST2_UI) \
      $(DEPENDENCIES_VST2_UI_BIN) \
    ) \
  ) \
  $(call fcheck,vst3,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_VST3) \
    $(DEPENDENCIES_VST3_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_VST3_UI_BIN)) \
  ) \
  $(EXTRA_DEPENDENCIES) \
  $(ARTIFACT_ID) \
  $(PLUGIN_SHARED)

MERGED_DEPENDENCIES_BIN        := \
  $(DEPENDENCIES_BIN) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_UI_BIN)) \
  $(call fcheck,clap,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_CLAP_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_CLAP_UI_BIN)) \
  ) \
  $(call fcheck,gst,$(ENABLED_FEATURES),$(DEPENDENCIES_GST_BIN)) \
  $(call fcheck,jack test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $(DEPENDENCIES_JACK_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_JACK_UI_BIN)) \
  ) \
  $(call fcheck,ladspa,$(DEPENDENCIES_LADSPA_BIN)) \
  $(call fcheck,lv2,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_LV2_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_LV2_UI_BIN)) \
  ) \
  $(call fcheck,vst2,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_VST2_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_VST2_UI_BIN)) \
  ) \
  $(call fcheck,vst3,$(ENABLED_FEATURES), \
    $(DEPENDENCIES_VST3_BIN) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_VST3_UI_BIN)) \
  )

UNIQ_DEPENDENCIES              := $(sort $(MERGED_DEPENDENCIES))
UNIQ_DEPENDENCIES_BIN          := $(sort $(MERGED_DEPENDENCIES_BIN))
UNIQ_PLUGIN_DEPENDENCIES       := $(sort $(PLUGIN_DEPENDENCIES))
HOST_UNIQ_DEPENDENCIES         := $(UNIQ_DEPENDENCIES)
HOST_UNIQ_DEPENDENCIES_BIN     := $(UNIQ_DEPENDENCIES_BIN)
HOST_UNIQ_PLUGIN_DEPENDENCIES  := $(UNIQ_PLUGIN_DEPENDENCIES)


DEPENDENCIES                    = $(filter-out $(UNIQ_DEPENDENCIES_BIN), $(UNIQ_DEPENDENCIES))
ALL_DEPENDENCIES                = $(sort, $(MODDIR_ALL_DEPENDENCIES) $(ROOTDIR_ALL_DEPENDENCIES))
DEPENDENCIES_BIN                = $(UNIQ_DEPENDENCIES_BIN)
PLUGIN_DEPENDENCIES             = $(UNIQ_PLUGIN_DEPENDENCIES)
HOST_DEPENDENCIES               = $(filter-out $(HOST_UNIQ_DEPENDENCIES_BIN), $(HOST_UNIQ_DEPENDENCIES))
HOST_DEPENDENCIES_BIN           = $(HOST_UNIQ_DEPENDENCIES_BIN)
HOST_PLUGIN_DEPENDENCIES        = $(HOST_UNIQ_PLUGIN_DEPENDENCIES)

#------------------------------------------------------------------------------
# Nested variables
SRC_DEPS                        = $(foreach dep, $(DEPENDENCIES), $(if $(findstring src,$($(dep)_TYPE)),$(dep)))
PLUG_DEPS                       = $(foreach dep, $(DEPENDENCIES) $(PLUGIN_DEPENDENCIES), $(if $(findstring plug,$($(dep)_TYPE)),$(dep)))

HOST_SRC_DEPS                   = $(foreach dep, $(HOST_DEPENDENCIES), $(if $(findstring src,$($(dep)_TYPE)),HOST_$(dep)))
HOST_PLUG_DEPS                  = $(foreach dep, $(HOST_DEPENDENCIES) $(HOST_PLUGIN_DEPENDENCIES), $(if $(findstring plug,$($(dep)_TYPE)),HOST_$(dep)))

#------------------------------------------------------------------------------
# Miscellaneous names and paths
ifeq ($(PLATFORM),Windows)
  GSTREAMER_INSTDIR      := $(LIBDIR)/gstreamer-1.0
  GSTREAMER_PKGDIR       := $(LIBDIR)/gstreamer-1.0
else
  GSTREAMER_INSTDIR      := $(call fcheck,gst,$(ENABLED_FEATURES),$(shell pkg-config --variable=pluginsdir gstreamer-1.0),$(LIBDIR)/gstreamer-1.0)
  GSTREAMER_PKGDIR       := $(LIBDIR)/gstreamer-1.0
endif

PLUGIN_ARTIFACT_ID     := $(ROOT_ARTIFACT_ID)
PLUGIN_ARTIFACT_NAME   := $(if $(PLUGIN_PACKAGE_NAME),$(PLUGIN_PACKAGE_NAME),$($(ROOT_ARTIFACT_ID)_NAME))
PLUGIN_ARTIFACT_PREFIX := $(if $(PLUGIN_ARTIFACT_NAME),$(PLUGIN_ARTIFACT_NAME)-,)
PLUGIN_GROUP_NAME       = $(if $(PLUGIN_ARTIFACT_NAME),$(PLUGIN_ARTIFACT_NAME),)
PLUGIN_SHARED_NAME     ?= $($(ROOT_ARTIFACT_ID)_NAME)
INSTALL_LIBDIR         := $($(ROOT_ARTIFACT_ID)_NAME)
INSTALL_GROUP_LIBDIR   := $(if $(PLUGIN_GROUP_NAME),$(PLUGIN_GROUP_NAME),$(INSTALL_LIBDIR))

CLAP_LIBDIR            := $(LIBDIR)/clap
GST_LIBDIR             := $(LIBDIR)/$(INSTALL_GROUP_LIBDIR)
JACK_LIBDIR            := $(LIBDIR)/$(INSTALL_GROUP_LIBDIR)
LADSPA_LIBDIR          := $(LIBDIR)/ladspa
LV2_LIBDIR             := $(LIBDIR)/lv2/$(INSTALL_LIBDIR).lv2
VST2_LIBDIR            := $(LIBDIR)/vst/$(INSTALL_LIBDIR).vst
VST3_LIBDIR            := $(LIBDIR)/vst3/$(INSTALL_LIBDIR).vst3
DOC_SHAREDDIR          := $(SHAREDDIR)/doc/$($(ROOT_ARTIFACT_ID)_NAME)

ifeq ($(PLATFORM),Linux)
  ifeq ($(ARCHITECTURE_FAMILY),ia32)
    VST3_ARCH              := i386-linux
  else ifeq ($(ARCHITECTURE_FAMILY),x86_64)
    VST3_ARCH              := x86_64-linux
  else ifeq ($(ARCHITECTURE_FAMILY),arm32)
    VST3_ARCH              := armv7l-linux
  else ifeq ($(ARCHITECTURE_FAMILY),aarch64)
    VST3_ARCH              := armv8l-linux
  else
    VST3_ARCH              := $(shell uname -m)-linux
  endif
  VST3_EXT               := .so
  VST3_LDFLAGS           :=
else ifeq ($(PLATFORM),BSD)
  ifeq ($(ARCHITECTURE_FAMILY),ia32)
    VST3_ARCH              := i386-freebsd
  else ifeq ($(ARCHITECTURE_FAMILY),x86_64)
    VST3_ARCH              := x86_64-freebsd
  else ifeq ($(ARCHITECTURE_FAMILY),arm32)
    VST3_ARCH              := armv7-freebsd
  else ifeq ($(ARCHITECTURE_FAMILY),aarch64)
    VST3_ARCH              := armv8-freebsd
  else
    VST3_ARCH              := $(shell uname -m)-freebsd
  endif
  VST3_EXT               := .so
  VST3_LDFLAGS           :=
else ifeq ($(PLATFORM),Windows)
  ifeq ($(ARCHITECTURE_FAMILY),ia32)
    VST3_ARCH              := x86-win
  else ifeq ($(ARCHITECTURE_FAMILY),x86_64)
    VST3_ARCH              := x86_64-win
  else ifeq ($(ARCHITECTURE_FAMILY),arm32)
    VST3_ARCH              := arm-win
  else ifeq ($(ARCHITECTURE_FAMILY),aarch64)
    VST3_ARCH              := arm64x-win
  else
    VST3_ARCH              := $(PROCESSOR_ARCHITECTURE)-win
  endif
  VST3_EXT               := .vst3
  VST3_LDFLAGS           := -Wl,--kill-at
else ifeq ($(PLATFORM),MacOS)
  VST3_ARCH              := MacOS
  VST3_EXT               := .dylib
else
  VST3_ARCH              := $(shell uname -m)-${shell uname -s | tr '[:upper:]' '[:lower:]'}
  VST3_EXT               := $(LIBRARY_EXT)
endif

# Versioning
PLUGIN_PACKAGE_VERSION     := $($(ROOT_ARTIFACT_ID)_VERSION)
PLUGIN_PACKAGE_MAJOR       := $(call vmajor,$(PLUGIN_PACKAGE_VERSION))
PLUGIN_PACKAGE_MINOR       := $(call vminor,$(PLUGIN_PACKAGE_VERSION))
PLUGIN_PACKAGE_MICRO       := $(call vmicro,$(PLUGIN_PACKAGE_VERSION))
PLUGIN_PACKAGE_BRANCH      := $(call vbranch,$(PLUGIN_PACKAGE_VERSION))

VERSION_CFLAGS             := \
  -DLSP_PLUGIN_ARTIFACT_GROUP=\"$(PLUGIN_GROUP_NAME)\" \
  -DLSP_PLUGIN_ARTIFACT_NAME=\"$(PLUGIN_SHARED_NAME)\" \
  -DLSP_PLUGIN_PACKAGE_MAJOR=$(PLUGIN_PACKAGE_MAJOR) \
  -DLSP_PLUGIN_PACKAGE_MINOR=$(PLUGIN_PACKAGE_MINOR) \
  -DLSP_PLUGIN_PACKAGE_MICRO=$(PLUGIN_PACKAGE_MICRO) \
  -DLSP_PLUGIN_PACKAGE_BRANCH=\"$(PLUGIN_PACKAGE_BRANCH)\" \
  -DLSP_INSTALL_PREFIX=\"$(PREFIX)\"

#------------------------------------------------------------------------------
# Framework settings for (cross) build
LSP_PLUGIN_FW_SRC_RES               = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-res.cpp
LSP_PLUGIN_FW_SRC_RES_EXCL          = -x "fonts/**/:i18n/**/:models/**/:presets/**/:schema/**/:ui/**/"
LSP_PLUGIN_FW_SRC_RES_CKSUM         = $(LSP_PLUGIN_FW_BIN)/res-checksum.json

LSP_PLUGIN_FW_SRC_UI_RES            = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui-res.cpp
LSP_PLUGIN_FW_SRC_UI_RES_CKSUM      = $(LSP_PLUGIN_FW_BIN)/ui-res-checksum.json
LSP_PLUGIN_FW_SRC_UI_RES_EXCL       = 

LSP_PLUGIN_FW_DEPS                  = $(filter-out $(ARTIFACT_ID), $(DEPENDENCIES))
LSP_PLUGIN_FW_OBJ_CORE              = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-core.o
LSP_PLUGIN_FW_OBJ_META              = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-meta.o
LSP_PLUGIN_FW_OBJ_DSP               = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-dsp.o
LSP_PLUGIN_FW_OBJ_UI                = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui.o
LSP_PLUGIN_FW_OBJ_CTL               = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ctl.o
LSP_PLUGIN_FW_OBJ_WRAP_CAIRO        = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-wrap-cairo.o
LSP_PLUGIN_FW_OBJ_TEST              = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-test.o
LSP_PLUGIN_FW_OBJ_RES               = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-res.o
LSP_PLUGIN_FW_OBJ_UI_RES            = $(LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui-res.o

LSP_PLUGIN_FW_OBJ_VARS              = \
  -d "ARTIFACT_ID=$($(ROOT_ARTIFACT_ID)_NAME)" \
  -d "ARTIFACT_DESC=$($(ROOT_ARTIFACT_ID)_DESC)" \
  -d "ARTIFACT_VERSION=$($(ROOT_ARTIFACT_ID)_VERSION)"
LSP_PLUGIN_FW_OBJ                   = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(call fcheck,test ui,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $(LSP_PLUGIN_FW_OBJ_UI)\
    $(LSP_PLUGIN_FW_OBJ_CTL) \
  )

LSP_PLUGIN_FW_ALL_MFLAGS           += $(foreach dep,$(DEPENDENCIES),-DUSE_$(dep)) 
LSP_PLUGIN_FW_ALL_CFLAGS           += $(foreach dep,$(DEPENDENCIES), $($(dep)_CFLAGS))
LSP_PLUGIN_FW_ALL_CFLAGS           += $(foreach feature,$(ENABLED_FEATURES_DEFS), -DWITH_$(feature)_FEATURE)

# Framework settings for host build
HOST_LSP_PLUGIN_FW_SRC_RES          = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-res.cpp
HOST_LSP_PLUGIN_FW_SRC_RES_EXCL     = $(LSP_PLUGIN_FW_SRC_RES_EXCL)
HOST_LSP_PLUGIN_FW_SRC_RES_CKSUM    = $(HOST_LSP_PLUGIN_FW_BIN)/res-checksum.json

HOST_LSP_PLUGIN_FW_SRC_UI_RES       = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui-res.cpp
HOST_LSP_PLUGIN_FW_SRC_UI_RES_EXCL  = $(LSP_PLUGIN_FW_SRC_UI_RES_EXCL)
HOST_LSP_PLUGIN_FW_SRC_UI_RES_CKSUM = $(HOST_LSP_PLUGIN_FW_BIN)/ui-res-checksum.json

HOST_LSP_PLUGIN_FW_DEPS             = $(filter-out $(ARTIFACT_ID), $(HOST_DEPENDENCIES))
HOST_LSP_PLUGIN_FW_OBJ_CORE         = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-core.o
HOST_LSP_PLUGIN_FW_OBJ_META         = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-meta.o
HOST_LSP_PLUGIN_FW_OBJ_DSP          = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-dsp.o
HOST_LSP_PLUGIN_FW_OBJ_UI           = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui.o
HOST_LSP_PLUGIN_FW_OBJ_CTL          = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ctl.o
HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO   = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-wrap-cairo.o
HOST_LSP_PLUGIN_FW_OBJ_TEST         = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-test.o
HOST_LSP_PLUGIN_FW_OBJ_RES          = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-res.o
HOST_LSP_PLUGIN_FW_OBJ_UI_RES       = $(HOST_LSP_PLUGIN_FW_BIN)/$(LSP_PLUGIN_FW_NAME)-ui-res.o
HOST_LSP_PLUGIN_FW_OBJ              = \
  $(HOST_LSP_PLUGIN_FW_OBJ_CORE) \
  $(HOST_LSP_PLUGIN_FW_OBJ_META) \
  $(HOST_LSP_PLUGIN_FW_OBJ_DSP) \
  $(call fcheck,test ui,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $(HOST_LSP_PLUGIN_FW_OBJ_UI) \
    $(HOST_LSP_PLUGIN_FW_OBJ_CTL) \
  )
  
HOST_LSP_PLUGIN_FW_ALL_MFLAGS      += $(foreach dep,$(DEPENDENCIES),-DUSE_$(dep)) 
HOST_LSP_PLUGIN_FW_ALL_CFLAGS      += $(foreach dep,$(DEPENDENCIES), $(HOST_$(dep)_CFLAGS))
HOST_LSP_PLUGIN_FW_ALL_CFLAGS      += $(foreach feature,$(ENABLED_FEATURES_DEFS), -DWITH_$(feature)_FEATURE)

# Source code location for (cross) build
CXX_SRC_CORE                = $(call rwildcard, main/core, *.cpp)
CXX_SRC_META                = $(call rwildcard, main/meta, *.cpp)
CXX_SRC_DSP                 = $(call rwildcard, main/plug, *.cpp)
CXX_SRC_UI                  = $(call rwildcard, main/ui, *.cpp)
CXX_SRC_CTL                 = $(call rwildcard, main/ctl, *.cpp)
CXX_SRC_EXPORT              = $(call rwildcard, export, *.cpp)
CXX_SRC_WRAP_JACK           = wrap/jack.cpp
CXX_SRC_WRAP_GST            = wrap/gstreamer.cpp
CXX_SRC_WRAP_LADSPA         = wrap/ladspa.cpp
CXX_SRC_WRAP_LV2            = wrap/lv2.cpp
CXX_SRC_WRAP_LV2_UI         = wrap/lv2ui.cpp
CXX_SRC_WRAP_VST2           = wrap/vst2.cpp
CXX_SRC_WRAP_VST3           = wrap/vst3.cpp
CXX_SRC_WRAP_CLAP           = wrap/clap.cpp
CXX_SRC_WRAP_CAIRO          = $(call rwildcard, wrap/cairo, *.cpp)
CXX_SRC_UTIL                = $(call rwildcard, util, *.cpp)
CXX_SRC_TEST                = $(call rwildcard, test, *.cpp)
CXX_SRC                     = \
  $(CXX_SRC_CORE) \
  $(CXX_SRC_EXPORT) \
  $(CXX_SRC_META) \
  $(CXX_SRC_DSP) \
  $(call fcheck,ui test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $(CXX_SRC_UI) \
    $(CXX_SRC_CTL) \
  ) \
  $(call fcheck,jack test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(CXX_SRC_WRAP_JACK)) \
  $(call fcheck,gst,$(ENABLED_FEATURES),$(CXX_SRC_WRAP_GST)) \
  $(call fcheck,ladspa,$(ENABLED_FEATURES),$(CXX_SRC_WRAP_LADSPA)) \
  $(call fcheck,lv2,$(ENABLED_FEATURES), \
    $(CXX_SRC_WRAP_LV2) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(CXX_SRC_WRAP_LV2_UI)) \
  ) \
  $(call fcheck,vst2,$(ENABLED_FEATURES),$(CXX_SRC_WRAP_VST2)) \
  $(call fcheck,vst3,$(ENABLED_FEATURES),$(CXX_SRC_WRAP_VST3)) \
  $(call fcheck,clap,$(ENABLED_FEATURES),$(CXX_SRC_WRAP_CLAP)) \
  $(if $(LIBCAIRO_VERSION),$(CXX_SRC_WRAP_CAIRO)) \
  $(CXX_SRC_UTIL)

# Source code location for host
HOST_CXX_SRC_CORE           = $(CXX_SRC_CORE)
HOST_CXX_SRC_EXPORT         = $(CXX_SRC_EXPORT)
HOST_CXX_SRC_META           = $(CXX_SRC_META)
HOST_CXX_SRC_DSP            = $(CXX_SRC_DSP)
HOST_CXX_SRC_UI             = $(CXX_SRC_UI)
HOST_CXX_SRC_CTL            = $(CXX_SRC_CTL)
HOST_CXX_SRC_WRAP_JACK      = $(CXX_SRC_WRAP_JACK)
HOST_CXX_SRC_WRAP_LADSPA    = $(CXX_SRC_WRAP_LADSPA)
HOST_CXX_SRC_WRAP_LV2       = $(CXX_SRC_WRAP_LV2)
HOST_CXX_SRC_WRAP_LV2_UI    = $(CXX_SRC_WRAP_LV2_UI)
HOST_CXX_SRC_WRAP_VST2      = $(CXX_SRC_WRAP_VST2)
HOST_CXX_SRC_WRAP_VST3      = $(CXX_SRC_WRAP_VST3)
HOST_CXX_SRC_WRAP_CLAP      = $(CXX_SRC_WRAP_CLAP)
HOST_CXX_SRC_WRAP_CAIRO     = $(CXX_SRC_WRAP_CAIRO)
HOST_CXX_SRC_UTIL           = $(CXX_SRC_UTIL)
HOST_CXX_SRC_TEST           = $(CXX_SRC_TEST)
HOST_CXX_SRC                = \
  $(HOST_CXX_SRC_CORE) \
  $(HOST_CXX_SRC_EXPORT) \
  $(HOST_CXX_SRC_META) \
  $(HOST_CXX_SRC_DSP) \
  $(call fcheck,ui test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $(HOST_CXX_SRC_UI) \
    $(HOST_CXX_SRC_CTL) \
  ) \
  $(call fcheck,jack test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(HOST_CXX_SRC_WRAP_JACK)) \
  $(call fcheck,gst,$(ENABLED_FEATURES),$(HOST_CXX_SRC_WRAP_GST)) \
  $(call fcheck,ladspa,$(ENABLED_FEATURES),$(HOST_CXX_SRC_WRAP_LADSPA)) \
  $(call fcheck,lv2,$(ENABLED_FEATURES), \
    $(HOST_CXX_SRC_WRAP_LV2) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(HOST_CXX_SRC_WRAP_LV2_UI)) \
  ) \
  $(call fcheck,vst2,$(ENABLED_FEATURES),$(HOST_CXX_SRC_WRAP_VST2)) \
  $(call fcheck,vst3,$(ENABLED_FEATURES),$(HOST_CXX_SRC_WRAP_VST3)) \
  $(call fcheck,clap,$(ENABLED_FEATURES),$(HOST_CXX_SRC_WRAP_CLAP)) \
  $(if $(LIBCAIRO_VERSION),$(HOST_CXX_SRC_WRAP_CAIRO)) \
  $(HOST_CXX_SRC_UTIL)

# Object files for (cross) build
OBJ_STUB                    = $(patsubst %.cpp, %.o, $(CXX_SRC_STUB))
OBJ_CORE                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_CORE))
OBJ_EXPORT                  = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_EXPORT))
OBJ_META                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_META))
OBJ_DSP                     = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_DSP))
OBJ_UI                      = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_UI))
OBJ_CTL                     = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_CTL))
OBJ_WRAP_CLAP               = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_CLAP))
OBJ_WRAP_GST                = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_GST))
OBJ_WRAP_JACK               = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_JACK))
OBJ_WRAP_LADSPA             = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_LADSPA))
OBJ_WRAP_LV2                = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_LV2))
OBJ_WRAP_LV2_UI             = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_LV2_UI))
OBJ_WRAP_VST2               = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_VST2))
OBJ_WRAP_VST3               = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_VST3))
OBJ_WRAP_CAIRO              = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_WRAP_CAIRO))
OBJ_UTIL                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_UTIL))
OBJ_TEST                    = $(patsubst %.cpp, $(LSP_PLUGIN_FW_BIN)/%.o, $(CXX_SRC_TEST))
OBJ                         = \
  $(OBJ_CORE) \
  $(OBJ_EXPORT) \
  $(OBJ_META) \
  $(OBJ_DSP) \
  $(call fcheck,ui test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $(OBJ_UI) \
    $(OBJ_CTL) \
  ) \
  $(OBJ_UTIL) \
  $(call fcheck,clap,$(ENABLED_FEATURES),$(OBJ_WRAP_CLAP)) \
  $(call fcheck,gst,$(ENABLED_FEATURES),$(OBJ_WRAP_GST)) \
  $(call fcheck,jack test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(OBJ_WRAP_JACK)) \
  $(call fcheck,ladspa test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(OBJ_WRAP_LADSPA)) \
  $(call fcheck,lv2,$(ENABLED_FEATURES), \
    $(OBJ_WRAP_LV2) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(OBJ_WRAP_LV2_UI)) \
  ) \
  $(call fcheck,vst2,$(ENABLED_FEATURES),$(OBJ_WRAP_VST2)) \
  $(call fcheck,vst3,$(ENABLED_FEATURES),$(OBJ_WRAP_VST3)) \
  $(if $(LIBCAIRO_VERSION),$(OBJ_WRAP_CAIRO))

# Object files for host build
HOST_OBJ_STUB               = $(patsubst %.cpp, %.o, $(HOST_CXX_SRC_STUB))
HOST_OBJ_CORE               = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_CORE))
HOST_OBJ_EXPORT             = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_EXPORT))
HOST_OBJ_META               = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_META))
HOST_OBJ_DSP                = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_DSP))
HOST_OBJ_UI                 = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_UI))
HOST_OBJ_CTL                = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_CTL))
HOST_OBJ_WRAP_CLAP          = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_CLAP))
HOST_OBJ_WRAP_GST           = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_GST))
HOST_OBJ_WRAP_JACK          = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_JACK))
HOST_OBJ_WRAP_LADSPA        = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_LADSPA))
HOST_OBJ_WRAP_LV2           = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_LV2))
HOST_OBJ_WRAP_LV2_UI        = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_LV2_UI))
HOST_OBJ_WRAP_VST2          = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_VST2))
HOST_OBJ_WRAP_VST3          = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_VST3))
HOST_OBJ_WRAP_CAIRO         = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_WRAP_CAIRO))
HOST_OBJ_UTIL               = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_UTIL))
HOST_OBJ_TEST               = $(patsubst %.cpp, $(HOST_LSP_PLUGIN_FW_BIN)/%.o, $(HOST_CXX_SRC_TEST))
HOST_OBJ                    = \
  $(HOST_OBJ_CORE) \
  $(HOST_OBJ_EXPORT) \
  $(HOST_OBJ_META) \
  $(HOST_OBJ_DSP) \
  $(call fcheck,ui test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $(HOST_OBJ_UI) \
    $(HOST_OBJ_CTL) \
  ) \
  $(HOST_OBJ_UTIL) \
  $(call fcheck,clap,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_CLAP)) \
  $(call fcheck,gst,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_GST)) \
  $(call fcheck,jack test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(HOST_OBJ_WRAP_JACK)) \
  $(call fcheck,ladspa test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(HOST_OBJ_WRAP_LADSPA)) \
  $(call fcheck,lv2,$(ENABLED_FEATURES), \
    $(HOST_OBJ_WRAP_LV2) \
    $(call fcheck,ui,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_LV2_UI)) \
  ) \
  $(call fcheck,vst2,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_VST2)) \
  $(call fcheck,vst3,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_VST3)) \
  $(if $(LIBCAIRO_VERSION),$(HOST_OBJ_WRAP_CAIRO))

ifeq ($(TEST),1)
  CXX_SRC                    += $(CXX_SRC_TEST)
  OBJ                        += $(OBJ_TEST)
  LSP_PLUGIN_FW_OBJ          += $(LSP_PLUGIN_FW_OBJ_TEST)
  
  HOST_CXX_SRC               += $(HOST_CXX_SRC_TEST)
  HOST_OBJ                   += $(HOST_OBJ_TEST)
  HOST_LSP_PLUGIN_FW_OBJ     += $(HOST_LSP_PLUGIN_FW_OBJ_TEST)
endif

#------------------------------------------------------------------------------
# Utilities
$(HOST)UTL_BIN_PATH               = $($(HOST)LSP_PLUGIN_FW_BIN)/utils-bin
$(HOST)UTL_RES_PATH               = $($(HOST)LSP_PLUGIN_FW_BIN)/res

$(HOST)UTL_COMMON_OBJ             = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/common, *.cpp))

$(HOST)UTL_VALIDATOR              = $($(HOST)UTL_BIN_PATH)/validator$(EXECUTABLE_EXT)
$(HOST)UTL_VALIDATOR_OBJ          = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/validator, *.cpp))
$(HOST)UTL_VALIDATOR_MAIN_OBJ     = $($(HOST)LSP_PLUGIN_FW_BIN)/util/validator.o
$(HOST)UTL_VALIDATOR_DEPS_ALL     = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_VALIDATOR_DEPS         = $(foreach dep, $($(HOST)UTL_VALIDATOR_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_VALIDATOR_LIBS         = $(foreach dep, $($(HOST)UTL_VALIDATOR_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_VALIDATOR_LDFLAGS      = $(foreach dep, $($(HOST)UTL_VALIDATOR_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_VALIDATOR_OBJS         = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $(call fcheck,test ui,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $($(HOST)LSP_PLUGIN_FW_OBJ_UI) \
    $($(HOST)LSP_PLUGIN_FW_OBJ_CTL) \
  ) \
  $(call fcheck,clap gst jack ladspa lv2 vst2 vst3,$(ENABLED_FEATURES), \
    $($(HOST)LSP_PLUGIN_FW_OBJ_RES) \
  ) \
  $($(HOST)OBJ_PLUG_META) \
  $($(HOST)OBJ_PLUG_DSP) \
  $($(HOST)OBJ_PLUG_SHARED) \
  $(call fcheck,test ui,$(ENABLED_FEATURES) $(INTERNAL_FEATURES), \
    $($(HOST)OBJ_PLUG_UI) \
  ) \
  $($(HOST)UTL_VALIDATOR_OBJ) \
  $($(HOST)UTL_COMMON_OBJ) \
  $($(HOST)UTL_VALIDATOR_MAIN_OBJ)

$(HOST)UTL_JACK_MAKE              = $($(HOST)UTL_BIN_PATH)/jack_make$(EXECUTABLE_EXT)
$(HOST)UTL_JACK_MAKE_OBJ          = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/jack_make, *.cpp))
$(HOST)UTL_JACK_MAKE_MAIN_OBJ     = $($(HOST)LSP_PLUGIN_FW_BIN)/util/jack_make.o
$(HOST)UTL_JACK_MAKE_DEPS_ALL     = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_JACK_MAKE_DEPS         = $(foreach dep, $($(HOST)UTL_JACK_MAKE_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_JACK_MAKE_LIBS         = $(foreach dep, $($(HOST)UTL_JACK_MAKE_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_JACK_MAKE_LDFLAGS      = $(foreach dep, $($(HOST)UTL_JACK_MAKE_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_JACK_MAKE_OBJS         = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_RES) \
  $($(HOST)OBJ_PLUG_META) \
  $($(HOST)OBJ_PLUG_DSP) \
  $($(HOST)OBJ_PLUG_SHARED) \
  $($(HOST)UTL_COMMON_OBJ) \
  $($(HOST)UTL_JACK_MAKE_OBJ) \
  $($(HOST)UTL_JACK_MAKE_MAIN_OBJ)

$(HOST)UTL_GST_MAKE               = $($(HOST)UTL_BIN_PATH)/gst_make$(EXECUTABLE_EXT)
$(HOST)UTL_GST_MAKE_OBJ           = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/gst_make, *.cpp))
$(HOST)UTL_GST_MAKE_MAIN_OBJ      = $($(HOST)LSP_PLUGIN_FW_BIN)/util/gst_make.o
$(HOST)UTL_GST_MAKE_DEPS_ALL      = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_GST_MAKE_DEPS          = $(foreach dep, $($(HOST)UTL_GST_MAKE_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_GST_MAKE_LIBS          = $(foreach dep, $($(HOST)UTL_GST_MAKE_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_GST_MAKE_LDFLAGS       = $(foreach dep, $($(HOST)UTL_GST_MAKE_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_GST_MAKE_OBJS          = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_RES) \
  $($(HOST)OBJ_PLUG_META) \
  $($(HOST)OBJ_PLUG_DSP) \
  $($(HOST)OBJ_PLUG_SHARED) \
  $($(HOST)UTL_COMMON_OBJ) \
  $($(HOST)UTL_GST_MAKE_OBJ) \
  $($(HOST)UTL_GST_MAKE_MAIN_OBJ)

$(HOST)UTL_VST2_MAKE              = $($(HOST)UTL_BIN_PATH)/vst2_make$(EXECUTABLE_EXT)
$(HOST)UTL_VST2_MAKE_OBJ          = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/vst2_make, *.cpp))
$(HOST)UTL_VST2_MAKE_MAIN_OBJ     = $($(HOST)LSP_PLUGIN_FW_BIN)/util/vst2_make.o
$(HOST)UTL_VST2_MAKE_DEPS_ALL     = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_VST2_MAKE_DEPS         = $(foreach dep, $($(HOST)UTL_VST2_MAKE_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_VST2_MAKE_LIBS         = $(foreach dep, $($(HOST)UTL_VST2_MAKE_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_VST2_MAKE_LDFLAGS      = $(foreach dep, $($(HOST)UTL_VST2_MAKE_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_VST2_MAKE_OBJS         = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_RES) \
  $($(HOST)OBJ_PLUG_META) \
  $($(HOST)OBJ_PLUG_DSP) \
  $($(HOST)OBJ_PLUG_SHARED) \
  $($(HOST)UTL_COMMON_OBJ) \
  $($(HOST)UTL_VST2_MAKE_OBJ) \
  $($(HOST)UTL_VST2_MAKE_MAIN_OBJ)
  
$(HOST)UTL_VST3_MODINFO           = $($(HOST)UTL_BIN_PATH)/vst3_modinfo$(EXECUTABLE_EXT)
$(HOST)UTL_VST3_MODINFO_OBJ       = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/vst3_modinfo, *.cpp))
$(HOST)UTL_VST3_MODINFO_MAIN_OBJ  = $($(HOST)LSP_PLUGIN_FW_BIN)/util/vst3_modinfo.o
$(HOST)UTL_VST3_MODINFO_DEPS_ALL  = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_VST3_MODINFO_DEPS      = $(foreach dep, $($(HOST)UTL_VST3_MODINFO_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_VST3_MODINFO_LIBS      = $(foreach dep, $($(HOST)UTL_VST3_MODINFO_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_VST3_MODINFO_LDFLAGS   = $(foreach dep, $($(HOST)UTL_VST3_MODINFO_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_VST3_MODINFO_OBJS      = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_RES) \
  $($(HOST)OBJ_PLUG_META) \
  $($(HOST)OBJ_PLUG_DSP) \
  $($(HOST)OBJ_PLUG_SHARED) \
  $($(HOST)UTL_COMMON_OBJ) \
  $($(HOST)UTL_VST3_MODINFO_OBJ) \
  $($(HOST)UTL_VST3_MODINFO_MAIN_OBJ)

$(HOST)UTL_LV2TTL_GEN             = $($(HOST)UTL_BIN_PATH)/lv2ttl_gen$(EXECUTABLE_EXT)
$(HOST)UTL_LV2TTL_GEN_OBJ         = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/lv2ttl_gen, *.cpp))
$(HOST)UTL_LV2TTL_GEN_MAIN_OBJ    = $($(HOST)LSP_PLUGIN_FW_BIN)/util/lv2ttl_gen.o
$(HOST)UTL_LV2TTL_GEN_DEPS_ALL    = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_LV2TTL_GEN_DEPS        = $(foreach dep, $($(HOST)UTL_LV2TTL_GEN_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_LV2TTL_GEN_LIBS        = $(foreach dep, $($(HOST)UTL_LV2TTL_GEN_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_LV2TTL_GEN_LDFLAGS     = $(foreach dep, $($(HOST)UTL_LV2TTL_GEN_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_LV2TTL_GEN_OBJS        = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_RES) \
  $($(HOST)OBJ_PLUG_META) \
  $($(HOST)OBJ_PLUG_DSP) \
  $($(HOST)OBJ_PLUG_SHARED) \
  $($(HOST)UTL_LV2TTL_GEN_OBJ) \
  $($(HOST)UTL_LV2TTL_GEN_MAIN_OBJ)

$(HOST)UTL_REPOSITORY             = $($(HOST)UTL_BIN_PATH)/repository$(EXECUTABLE_EXT)
$(HOST)UTL_REPOSITORY_OBJ         = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/repository, *.cpp))
$(HOST)UTL_REPOSITORY_MAIN_OBJ    = $($(HOST)LSP_PLUGIN_FW_BIN)/util/repository.o
$(HOST)UTL_REPOSITORY_DEPS_ALL    = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_REPOSITORY_DEPS        = $(foreach dep, $($(HOST)UTL_REPOSITORY_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_REPOSITORY_LIBS        = $(foreach dep, $($(HOST)UTL_REPOSITORY_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_REPOSITORY_LDFLAGS     = $(foreach dep, $($(HOST)UTL_REPOSITORY_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_REPOSITORY_OBJS        = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $($(HOST)UTL_COMMON_OBJ) \
  $($(HOST)UTL_REPOSITORY_OBJ) \
  $($(HOST)UTL_REPOSITORY_MAIN_OBJ)

$(HOST)UTL_RESPACK                = $($(HOST)UTL_BIN_PATH)/respack$(EXECUTABLE_EXT)
$(HOST)UTL_RESPACK_OBJ            = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/respack, *.cpp))
$(HOST)UTL_RESPACK_MAIN_OBJ       = $($(HOST)LSP_PLUGIN_FW_BIN)/util/respack.o
$(HOST)UTL_RESPACK_DEPS_ALL       = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_RESPACK_DEPS           = $(foreach dep, $($(HOST)UTL_RESPACK_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_RESPACK_LIBS           = $(foreach dep, $($(HOST)UTL_RESPACK_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_RESPACK_LDFLAGS        = $(foreach dep, $($(HOST)UTL_RESPACK_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_RESPACK_OBJS           = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $($(HOST)UTL_COMMON_OBJ) \
  $($(HOST)UTL_RESPACK_OBJ) \
  $($(HOST)UTL_RESPACK_MAIN_OBJ)
  
$(HOST)UTL_PLUGLIST_GEN           = $($(HOST)UTL_BIN_PATH)/pluglist_gen$(EXECUTABLE_EXT)
$(HOST)UTL_PLUGLIST_GEN_OBJ       = $(patsubst %.cpp,$($(HOST)LSP_PLUGIN_FW_BIN)/%.o,$(call rwildcard, util/pluglist_gen, *.cpp))
$(HOST)UTL_PLUGLIST_GEN_MAIN_OBJ  = $($(HOST)LSP_PLUGIN_FW_BIN)/util/pluglist_gen.o
$(HOST)UTL_PLUGLIST_GEN_DEPS_ALL  = $(call uniq, $($(HOST)LSP_PLUGIN_FW_DEPS))
$(HOST)UTL_PLUGLIST_GEN_DEPS      = $(foreach dep, $($(HOST)UTL_PLUGLIST_GEN_DEPS_ALL), $(if $($(HOST)$(dep)_OBJ), $(HOST)$(dep)))
$(HOST)UTL_PLUGLIST_GEN_LIBS      = $(foreach dep, $($(HOST)UTL_PLUGLIST_GEN_DEPS_ALL), $($(HOST)$(dep)_OBJ))
$(HOST)UTL_PLUGLIST_GEN_LDFLAGS   = $(foreach dep, $($(HOST)UTL_PLUGLIST_GEN_DEPS_ALL), $($(HOST)$(dep)_LDFLAGS))
$(HOST)UTL_PLUGLIST_GEN_OBJS      = \
  $($(HOST)LSP_PLUGIN_FW_OBJ_CORE) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_META) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_DSP) \
  $($(HOST)LSP_PLUGIN_FW_OBJ_RES) \
  $($(HOST)OBJ_PLUG_META) \
  $($(HOST)OBJ_PLUG_DSP) \
  $($(HOST)OBJ_PLUG_SHARED) \
  $($(HOST)UTL_PLUGLIST_GEN_OBJ) \
  $($(HOST)UTL_PLUGLIST_GEN_MAIN_OBJ)

#------------------------------------------------------------------------------
# Plugins
OBJ_PLUG_META               = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_META))
OBJ_PLUG_DSP                = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_DSP))
OBJ_PLUG_SHARED             = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_SHARED))
OBJ_PLUG_UI                 = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_UI))
OBJ_PLUG_TEST               = $(foreach dep, $(PLUG_DEPS), $($(dep)_OBJ_TEST))
OBJ_PLUG                    = $(OBJ_PLUG_META) $(OBJ_PLUG_DSP) $(OBJ_PLUG_SHARED) $(OBJ_PLUG_UI)

HOST_OBJ_PLUG_META          = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_META))
HOST_OBJ_PLUG_DSP           = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_DSP))
HOST_OBJ_PLUG_SHARED        = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_SHARED))
HOST_OBJ_PLUG_UI            = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_UI))
HOST_OBJ_PLUG_TEST          = $(foreach dep, $(HOST_PLUG_DEPS), $($(dep)_OBJ_TEST))
HOST_OBJ_PLUG               = $(HOST_OBJ_PLUG_META) $(HOST_OBJ_PLUG_DSP) $(HOST_OBJ_PLUG_SHARED) $(HOST_OBJ_PLUG_UI)

#------------------------------------------------------------------------------
# Binaries
ARTIFACT_LIB_CLAP           = $(LSP_PLUGIN_FW_BIN)/$(PLUGIN_SHARED_NAME).clap
ARTIFACT_LIB_CLAP_DEPS_FULL = \
  $(DEPENDENCIES_CLAP) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_CLAP_UI)) \
  $(DEPENDENCIES_TEST) \
  $(DEPENDENCIES_TEST_UI) \
  $(PLUGIN_SHARED)
ARTIFACT_LIB_CLAP_DEPS_ALL  = $(filter-out $(ARTIFACT_ID), $(call uniq, $(ARTIFACT_LIB_CLAP_DEPS_FULL)))
ARTIFACT_LIB_CLAP_DEPS      = $(foreach dep, $(ARTIFACT_LIB_CLAP_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_CLAP_LIBS      = $(foreach dep, $(ARTIFACT_LIB_CLAP_DEPS_ALL), $($(dep)_OBJ))
ARTIFACT_LIB_CLAP_LDFLAGS   = $(foreach dep, $(ARTIFACT_LIB_CLAP_DEPS_ALL), $($(dep)_LDFLAGS))
ARTIFACT_LIB_CLAP_OBJS      = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(call fcheck,ui,$(ENABLED_FEATURES), \
    $(LSP_PLUGIN_FW_OBJ_UI) \
    $(LSP_PLUGIN_FW_OBJ_CTL) \
    $(LSP_PLUGIN_FW_OBJ_UI_RES), \
    $(LSP_PLUGIN_FW_OBJ_RES) \
  ) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_SHARED) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(OBJ_PLUG_UI)) \
  $(OBJ_WRAP_CLAP)

ARTIFACT_LIB_JACK           = $(LSP_PLUGIN_FW_BIN)/$(LIBRARY_PREFIX)$(PLUGIN_SHARED_NAME)-jack-$(PLUGIN_PACKAGE_VERSION)$(LIBRARY_EXT)
ARTIFACT_LIB_JACK_PATH      = $(LSP_PLUGIN_FW_BIN)/jack
ARTIFACT_LIB_JACK_SETTINGS  = $(ARTIFACT_LIB_JACK_PATH)/.settings.mk
ARTIFACT_LIB_JACK_DEPS_FULL = \
  $(DEPENDENCIES_JACK) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_JACK_UI)) \
  $(DEPENDENCIES_TEST) \
  $(DEPENDENCIES_TEST_UI) \
  $(PLUGIN_SHARED)
ARTIFACT_LIB_JACK_DEPS_ALL  = $(filter-out $(ARTIFACT_ID), $(call uniq, $(ARTIFACT_LIB_JACK_DEPS_FULL)))
ARTIFACT_LIB_JACK_DEPS      = $(foreach dep, $(ARTIFACT_LIB_JACK_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_JACK_LIBS      = $(foreach dep, $(ARTIFACT_LIB_JACK_DEPS_ALL), $($(dep)_OBJ))
ARTIFACT_LIB_JACK_LDFLAGS   = $(foreach dep, $(ARTIFACT_LIB_JACK_DEPS_ALL), $($(dep)_LDFLAGS))
ARTIFACT_LIB_JACK_OBJS      = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(call fcheck,ui,$(ENABLED_FEATURES), \
    $(LSP_PLUGIN_FW_OBJ_UI) \
    $(LSP_PLUGIN_FW_OBJ_CTL) \
    $(LSP_PLUGIN_FW_OBJ_UI_RES) \
    $(if $(LIBCAIRO_VERSION),$(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO)), \
    $(LSP_PLUGIN_FW_OBJ_RES) \
  ) \
  $(OBJ_EXPORT) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_SHARED) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(OBJ_PLUG_UI)) \
  $(OBJ_WRAP_JACK)
  
ARTIFACT_LIB_GST            = $(LSP_PLUGIN_FW_BIN)/$(LIBRARY_PREFIX)$(PLUGIN_SHARED_NAME)-gstreamer-$(PLUGIN_PACKAGE_VERSION)$(LIBRARY_EXT)
ARTIFACT_LIB_GST_PATH       = $(LSP_PLUGIN_FW_BIN)/gst
ARTIFACT_LIB_GST_SETTINGS   = $(ARTIFACT_LIB_GST_PATH)/.settings.mk
ARTIFACT_LIB_GST_DEPS_ALL   = $(filter-out $(ARTIFACT_ID), $(call uniq, $(DEPENDENCIES_GST) $(DEPENDENCIES_TEST) $(PLUGIN_SHARED)))
ARTIFACT_LIB_GST_DEPS       = $(foreach dep, $(ARTIFACT_LIB_GST_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_GST_LIBS       = $(foreach dep, $(ARTIFACT_LIB_GST_DEPS_ALL), $($(dep)_OBJ))
ARTIFACT_LIB_GST_LDFLAGS    = $(foreach dep, $(ARTIFACT_LIB_GST_DEPS_ALL), $($(dep)_LDFLAGS))
ARTIFACT_LIB_GST_OBJS       = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_RES) \
  $(OBJ_EXPORT) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_SHARED) \
  $(OBJ_WRAP_GST)
  
ARTIFACT_LIB_LADSPA         = $(LSP_PLUGIN_FW_BIN)/$(PLUGIN_SHARED_NAME)-ladspa$(LIBRARY_EXT)
ARTIFACT_LIB_LADSPA_DEPS_ALL= $(filter-out $(ARTIFACT_ID), $(call uniq, $(DEPENDENCIES_LADSPA) $(DEPENDENCIES_TEST) $(PLUGIN_SHARED)))
ARTIFACT_LIB_LADSPA_DEPS    = $(foreach dep, $(ARTIFACT_LIB_LADSPA_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_LADSPA_LIBS    = $(foreach dep, $(ARTIFACT_LIB_LADSPA_DEPS_ALL), $($(dep)_OBJ))
ARTIFACT_LIB_LADSPA_LDFLAGS = $(foreach dep, $(ARTIFACT_LIB_LADSPA_DEPS_ALL), $($(dep)_LDFLAGS))
ARTIFACT_LIB_LADSPA_OBJS    = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_RES) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_SHARED) \
  $(OBJ_WRAP_LADSPA)
  
ARTIFACT_LIB_LV2            = $(LSP_PLUGIN_FW_BIN)/$(PLUGIN_SHARED_NAME)-lv2$(LIBRARY_EXT)
ARTIFACT_LIB_LV2_DEPS_ALL   = $(filter-out $(ARTIFACT_ID), $(call uniq, $(DEPENDENCIES_LV2) $(DEPENDENCIES_TEST) $(PLUGIN_SHARED)))
ARTIFACT_LIB_LV2_DEPS       = $(foreach dep, $(ARTIFACT_LIB_LV2_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_LV2_LIBS       = $(foreach dep, $(ARTIFACT_LIB_LV2_DEPS_ALL), $($(dep)_OBJ))
ARTIFACT_LIB_LV2_LDFLAGS    = $(foreach dep, $(ARTIFACT_LIB_LV2_DEPS_ALL), $($(dep)_LDFLAGS))
ARTIFACT_LIB_LV2_OBJS       = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_RES) \
  $(if $(LIBCAIRO_VERSION),$(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO)) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_SHARED) \
  $(OBJ_WRAP_LV2)

ARTIFACT_LIB_LV2_UI         = $(LSP_PLUGIN_FW_BIN)/$(PLUGIN_SHARED_NAME)-lv2ui$(LIBRARY_EXT)
ARTIFACT_LIB_LV2_UI_DEPS_ALL= $(filter-out $(ARTIFACT_ID), $(call uniq, $(DEPENDENCIES_LV2_UI) $(DEPENDENCIES_TEST) $(DEPENDENCIES_TEST_UI) $(PLUGIN_SHARED)))
ARTIFACT_LIB_LV2_UI_DEPS    = $(foreach dep, $(ARTIFACT_LIB_LV2_UI_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_LV2_UI_LIBS    = $(foreach dep, $(ARTIFACT_LIB_LV2_UI_DEPS_ALL), $($(dep)_OBJ))
ARTIFACT_LIB_LV2_UI_LDFLAGS = $(foreach dep, $(ARTIFACT_LIB_LV2_UI_DEPS_ALL), $($(dep)_LDFLAGS))
ARTIFACT_LIB_LV2_UI_OBJS    = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(LSP_PLUGIN_FW_OBJ_UI_RES) \
  $(LSP_PLUGIN_FW_OBJ_UI) \
  $(LSP_PLUGIN_FW_OBJ_CTL) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_SHARED) \
  $(OBJ_PLUG_UI) \
  $(OBJ_WRAP_LV2_UI)
  
ARTIFACT_LIB_VST2           = $(LSP_PLUGIN_FW_BIN)/$(LIBRARY_PREFIX)$(PLUGIN_SHARED_NAME)-vst2$(LIBRARY_EXT)
ARTIFACT_LIB_VST2_PATH      = $(LSP_PLUGIN_FW_BIN)/vst2
ARTIFACT_LIB_VST2_SETTINGS  = $(ARTIFACT_LIB_VST2_PATH)/.settings.mk
ARTIFACT_LIB_VST2_DEPS_FULL = \
  $(DEPENDENCIES_VST2) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_VST2_UI)) \
  $(DEPENDENCIES_TEST) \
  $(DEPENDENCIES_TEST_UI) \
  $(PLUGIN_SHARED)
ARTIFACT_LIB_VST2_DEPS_ALL  = $(filter-out $(ARTIFACT_ID), $(call uniq, $(ARTIFACT_LIB_VST2_DEPS_FULL)))
ARTIFACT_LIB_VST2_DEPS      = $(foreach dep, $(ARTIFACT_LIB_VST2_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_VST2_LIBS      = $(foreach dep, $(ARTIFACT_LIB_VST2_DEPS_ALL), $($(dep)_OBJ))
ARTIFACT_LIB_VST2_LDFLAGS   = $(foreach dep, $(ARTIFACT_LIB_VST2_DEPS_ALL), $($(dep)_LDFLAGS))
ARTIFACT_LIB_VST2_OBJS      = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(call fcheck,ui,$(ENABLED_FEATURES), \
    $(LSP_PLUGIN_FW_OBJ_UI) \
    $(LSP_PLUGIN_FW_OBJ_CTL) \
    $(LSP_PLUGIN_FW_OBJ_UI_RES), \
    $(LSP_PLUGIN_FW_OBJ_RES) \
  ) \
  $(OBJ_EXPORT) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_SHARED) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(OBJ_PLUG_UI)) \
  $(OBJ_WRAP_VST2)

ARTIFACT_LIB_VST3           = $(LSP_PLUGIN_FW_BIN)/$(PLUGIN_SHARED_NAME)$(VST3_EXT)
ARTIFACT_LIB_VST3_DEPS_FULL = \
  $(DEPENDENCIES_VST3) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_VST3_UI)) \
  $(DEPENDENCIES_TEST) \
  $(DEPENDENCIES_TEST_UI) \
  $(PLUGIN_SHARED)
ARTIFACT_LIB_VST3_DEPS_ALL  = $(filter-out $(ARTIFACT_ID), $(call uniq, $(ARTIFACT_LIB_VST3_DEPS_FULL)))
ARTIFACT_LIB_VST3_DEPS      = $(foreach dep, $(ARTIFACT_LIB_VST3_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
ARTIFACT_LIB_VST3_LIBS      = $(foreach dep, $(ARTIFACT_LIB_VST3_DEPS_ALL), $($(dep)_OBJ))
ARTIFACT_LIB_VST3_LDFLAGS   = $(foreach dep, $(ARTIFACT_LIB_VST3_DEPS_ALL), $($(dep)_LDFLAGS)) $(VST3_LDFLAGS)
ARTIFACT_LIB_VST3_OBJS      = \
  $(LSP_PLUGIN_FW_OBJ_CORE) \
  $(LSP_PLUGIN_FW_OBJ_META) \
  $(LSP_PLUGIN_FW_OBJ_DSP) \
  $(call fcheck,ui,$(ENABLED_FEATURES), \
    $(LSP_PLUGIN_FW_OBJ_UI) \
    $(LSP_PLUGIN_FW_OBJ_CTL) \
    $(LSP_PLUGIN_FW_OBJ_UI_RES), \
    $(LSP_PLUGIN_FW_OBJ_RES) \
  ) \
  $(OBJ_EXPORT) \
  $(OBJ_PLUG_META) \
  $(OBJ_PLUG_DSP) \
  $(OBJ_PLUG_SHARED) \
  $(call fcheck,ui,$(ENABLED_FEATURES),$(OBJ_PLUG_UI)) \
  $(OBJ_WRAP_VST3)

ifeq ($(TEST),1)
  ARTIFACT_BIN_TEST           = $(HOST_LSP_PLUGIN_FW_BIN)/$(PLUGIN_SHARED_NAME)-test$(EXECUTABLE_EXT)
  ARTIFACT_BIN_TEST_PATH      = $(HOST_LSP_PLUGIN_FW_BIN)
  ARTIFACT_BIN_TEST_DEPS_ALL  = $(filter-out $(ARTIFACT_ID), $(call uniq, $(DEPENDENCIES_JACK) $(DEPENDENCIES_JACK_UI) $(DEPENDENCIES_TEST) $(DEPENDENCIES_TEST_UI) $(PLUGIN_SHARED)))
  ARTIFACT_BIN_TEST_DEPS      = $(foreach dep, $(ARTIFACT_BIN_TEST_DEPS_ALL), $(if $($(dep)_OBJ), $(dep)))
  ARTIFACT_BIN_TEST_LIBS      = $(foreach dep, $(ARTIFACT_BIN_TEST_DEPS_ALL), $($(dep)_OBJ))
  ARTIFACT_BIN_TEST_LDFLAGS   = $(foreach dep, $(ARTIFACT_BIN_TEST_DEPS_ALL), $($(dep)_LDFLAGS))
  ARTIFACT_BIN_TEST_OBJS      = \
    $(HOST_LSP_PLUGIN_FW_OBJ_CORE) \
    $(HOST_LSP_PLUGIN_FW_OBJ_META) \
    $(HOST_LSP_PLUGIN_FW_OBJ_DSP) \
    $(HOST_LSP_PLUGIN_FW_OBJ_UI) \
    $(HOST_LSP_PLUGIN_FW_OBJ_CTL) \
    $(call fcheck,ui,$(ENABLED_FEATURES), $(HOST_LSP_PLUGIN_FW_OBJ_UI_RES)) \
    $(HOST_LSP_PLUGIN_FW_OBJ_TEST) \
    $(if $(LIBCAIRO_VERSION),$(HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO)) \
    $(HOST_OBJ_PLUG_META) \
    $(HOST_OBJ_PLUG_DSP) \
    $(HOST_OBJ_PLUG_SHARED) \
    $(HOST_OBJ_PLUG_UI) \
    $(call fcheck,clap,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_CLAP)) \
    $(call fcheck,gst,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_GST)) \
    $(call fcheck,jack test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(HOST_OBJ_WRAP_JACK)) \
    $(call fcheck,ladspa test,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(HOST_OBJ_WRAP_LADSPA)) \
    $(call fcheck,lv2,$(ENABLED_FEATURES), \
      $(HOST_OBJ_WRAP_LV2) \
      $(call fcheck,ui,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_LV2UI)) \
    ) \
    $(call fcheck,vst2,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_VST2)) \
    $(call fcheck,vst3,$(ENABLED_FEATURES),$(HOST_OBJ_WRAP_VST3)) \
    $($(HOST)UTL_COMMON_OBJ) \
    $($(HOST)UTL_VALIDATOR_OBJ) \
    $($(HOST)UTL_JACK_MAKE_OBJ) \
    $($(HOST)UTL_VST2_MAKE_OBJ) \
    $($(HOST)UTL_VST3_MODINFO_OBJ) \
    $($(HOST)UTL_LV2TTL_GEN_OBJ) \
    $($(HOST)UTL_REPOSITORY_OBJ) \
    $($(HOST)UTL_RESPACK_OBJ) \
    $($(HOST)UTL_PLUGLIST_GEN_OBJ)
endif

ARTIFACT_DOC_SHARED         = $(foreach dep, $(sort $(PLUGIN_SHARED) $(PLUGIN_DEPENDENCIES)), $(if $(findstring src,$($(dep)_TYPE)),$(dep)))
ARTIFACT_DOC_PLUG           = $(foreach dep, $(sort $(PLUGIN_SHARED) $(PLUGIN_DEPENDENCIES)), $(if $(findstring plug,$($(dep)_TYPE)),$(dep)))
ARTIFACT_DOC_BLD_PATH       = $(LSP_PLUGIN_FW_BIN)/doc
ARTIFACT_DOC_PATH           = $(LSP_PLUGIN_FW_BIN)/html
ARTIFACT_DOC_DEPS_ALL       = $(foreach dep, $(sort $(ARTIFACT_DOC_SHARED) $(ARTIFACT_DOC_PLUG)), $(if $(wildcard $($(dep)_SRC)/doc/*),$(dep)))
ARTIFACT_DOC_DEPS_SRC_RES   = $(foreach dep, $(sort $(ARTIFACT_DOC_SHARED)), $(if $(wildcard $($(dep)_PATH)/res/doc/*),$(dep)))
ARTIFACT_DOC_DEPS_PLUG_RES  = $(foreach dep, $(sort $(ARTIFACT_DOC_PLUG)), $(if $(wildcard $($(dep)_PATH)/res/doc/screenshots/*),$(dep)))
ARTIFACT_DOC_SRC_ALL        = $(foreach dep, $(ARTIFACT_DOC_DEPS_ALL), $($(dep)_SRC)/doc/*)
ARTIFACT_DOC_SRC_RES        = $(foreach dep, $(ARTIFACT_DOC_DEPS_SRC_RES), $($(dep)_PATH)/res/doc/*)
ARTIFACT_DOC_PLUG_RES       = $(foreach dep, $(ARTIFACT_DOC_DEPS_PLUG_RES), $($(dep)_PATH)/res/doc/screenshots/*)
ARTIFACT_DOC_PKG_DIR        = $(PLUGIN_SHARED_NAME)-doc-$(PLUGIN_PACKAGE_VERSION)

ARTIFACT_XDG_SRC_DEPS       = $(foreach dep, $(sort $(PLUGIN_SHARED) $(PLUGIN_DEPENDENCIES)), $(if $(wildcard $($(dep)_PATH)/res/xdg/*),$(dep)))
ARTIFACT_XDG_FILES          = $(foreach dep, $(ARTIFACT_XDG_SRC_DEPS), $(call rwildcard,$($(dep)_PATH)/res/xdg/apps,*.desktop))
ARTIFACT_XDG_MAKEFILES      = $(foreach dep, $(ARTIFACT_XDG_SRC_DEPS), $(if $(wildcard $($(dep)_PATH)/res/xdg/Makefile),$(dep)))
ARTIFACT_XDG_LIST_FILE      = $(LSP_PLUGIN_FW_BIN)/xdg/$(sort $(ARTIFACT_XDG_MAKEFILES)).list
ARTIFACT_XDG_BATCH_FILE     = $(LSP_PLUGIN_FW_BIN)/xdg/$(sort $(ARTIFACT_XDG_MAKEFILES)).mk
ARTIFACT_XDG_INST           = $(foreach dep, $(ARTIFACT_XDG_MAKEFILES), xdg_install_$(dep))
ARTIFACT_XDG_UNINST         = $(foreach dep, $(ARTIFACT_XDG_MAKEFILES), xdg_uninstall_$(dep))
ARTIFACT_XDG_INST_ID        = $(patsubst xdg_install_%,%,$(@))
ARTIFACT_XDG_UNINST_ID      = $(patsubst xdg_uninstall_%,%,$(@))

BIN_INSTALL                 = $(foreach dep, $(UNIQ_DEPENDENCIES_BIN), install_$(dep))
BIN_UNINSTALL               = $(foreach dep, $(UNIQ_DEPENDENCIES_BIN), uninstall_$(dep))

BIN_PACKAGE                 = $(foreach dep, $(UNIQ_BIN_DEPENDENCIES), package_$(dep))
BIN_PACKAGE_CLAP_DEPS       = \
  $(DEPENDENCIES_BIN) \
  $(DEPENDENCIES_CLAP_BIN) \
  $(call fcheck,ui,$(ENABLED_FEATURES),\
    $(DEPENDENCIES_UI_BIN) \
    $(DEPENDENCIES_CLAP_UI_BIN) \
  )
BIN_PACKAGE_GST_DEPS        = \
  $(DEPENDENCIES_BIN) \
  $(DEPENDENCIES_GST_BIN)
BIN_PACKAGE_JACK_DEPS       = \
  $(DEPENDENCIES_BIN) \
  $(DEPENDENCIES_JACK_BIN) \
  $(call fcheck,ui,$(ENABLED_FEATURES),\
    $(DEPENDENCIES_UI_BIN) \
    $(DEPENDENCIES_JACK_UI_BIN) \
  )
BIN_PACKAGE_LADSPA_DEPS     = \
  $(DEPENDENCIES_BIN) \
  $(DEPENDENCIES_LADSPA_BIN)
BIN_PACKAGE_LV2_DEPS        = \
  $(DEPENDENCIES_BIN) \
  $(DEPENDENCIES_LV2_BIN) \
  $(call fcheck,ui,$(ENABLED_FEATURES),\
    $(DEPENDENCIES_UI_BIN) \
    $(DEPENDENCIES_LV2_UI_BIN) \
  )
BIN_PACKAGE_VST2_DEPS       = \
  $(DEPENDENCIES_BIN) \
  $(DEPENDENCIES_VST2_BIN) \
  $(call fcheck,ui,$(ENABLED_FEATURES),\
    $(DEPENDENCIES_UI_BIN) \
    $(DEPENDENCIES_VST2_UI_BIN) \
  )
BIN_PACKAGE_VST3_DEPS       = \
  $(DEPENDENCIES_BIN) \
  $(DEPENDENCIES_VST3_BIN) \
  $(call fcheck,ui,$(ENABLED_FEATURES),\
    $(DEPENDENCIES_UI_BIN) \
    $(DEPENDENCIES_VST3_UI_BIN) \
  )

BIN_PACKAGE_CLAP            = $(foreach dep, $(sort $(BIN_PACKAGE_CLAP_DEPS)), package_$(dep))
BIN_PACKAGE_GST             = $(foreach dep, $(sort $(BIN_PACKAGE_GST_DEPS)), package_$(dep))
BIN_PACKAGE_JACK            = $(foreach dep, $(sort $(BIN_PACKAGE_JACK_DEPS)), package_$(dep))
BIN_PACKAGE_LADSPA          = $(foreach dep, $(sort $(BIN_PACKAGE_LADSPA_DEPS)), package_$(dep))
BIN_PACKAGE_LV2             = $(foreach dep, $(sort $(BIN_PACKAGE_LV2_DEPS)), package_$(dep))
BIN_PACKAGE_VST2            = $(foreach dep, $(sort $(BIN_PACKAGE_VST2_DEPS)), package_$(dep))
BIN_PACKAGE_VST3            = $(foreach dep, $(sort $(BIN_PACKAGE_VST3_DEPS)), package_$(dep))

BIN_INST_ARTIFACT_ID        = $(patsubst install_%,%,$(@))
BIN_UNINST_ARTIFACT_ID      = $(patsubst uninstall_%,%,$(@))
BIN_PKG_ARTIFACT_ID         = $(patsubst package_%,%,$(@))
BIN_PKG_DIR                 = $(PLUGIN_SHARED_NAME)-$(patsubst package_%,%,$(@))-$(PLUGIN_PACKAGE_VERSION)-$(PLATFORM)-$(ARCHITECTURE)
BIN_PKG_FILES               = \
  $(wildcard $(ROOTDIR)/CHANGELOG*) \
  $(wildcard $(ROOTDIR)/COPYING*) \
  $(wildcard $(ROOTDIR)/*LICENSE*) \
  $(wildcard $(ROOTDIR)/*.md) \
  $(wildcard $(ROOTDIR)/*.txt)
BIN_EXTRA_SHARED            = $(foreach dep, $(call uniq,$(PLUGIN_SHARED) $(PLUGIN_DEPENDENCIES)), $(if $(findstring src,$($(dep)_TYPE)),$(dep)))
BIN_EXTRA_PLUG              = $(foreach dep, $(call uniq,$(PLUGIN_SHARED) $(PLUGIN_DEPENDENCIES)), $(if $(findstring plug,$($(dep)_TYPE)),$(dep)))
BIN_EXTRA_FILES             = $(foreach dep, $(BIN_EXTRA_SHARED) $(BIN_EXTRA_PLUG),\
  $(wildcard $($(dep)_PATH)/COPYING*) \
  $(wildcard $($(dep)_PATH)/*LICENSE*) \
)

ALLOWED_BUILD_TARGETS       = clap doc gst jack ladspa lv2 test vst2 vst3 xdg meta validate
ALLOWED_PACKAGE_TARGETS     = clap doc gst jack ladspa lv2 vst2 vst3
BUILD_TARGETS               = $(call intersection,$(foreach feature,$(ENABLED_FEATURES) $(INTERNAL_FEATURES),$(feature)),$(ALLOWED_BUILD_TARGETS))
INSTALL_TARGETS             = $(foreach feature,$(INSTALL_FEATURES),install_$(feature))
UNINSTALL_TARGETS           = $(foreach feature,$(INSTALL_FEATURES),uninstall_$(feature))
PACKAGE_TARGETS             = $(foreach feature,$(call intersection,$(ENABLED_FEATURES),$(ALLOWED_PACKAGE_TARGETS)),package_$(feature))

#------------------------------------------------------------------------------
# Functional variables
CXX_FILE                    = $(patsubst $(LSP_PLUGIN_FW_BIN)/%.o,%.cpp, $(@))
HOST_CXX_FILE               = $(patsubst $(HOST_LSP_PLUGIN_FW_BIN)/%.o,%.cpp, $(@))
DEP_FILE                    = $(patsubst %.o,%.d, $(@))

#------------------------------------------------------------------------------
# Plugin instance settings
WRAP_GST_CFLAGS            := $(VERSION_CFLAGS) -DLSP_PLUGIN_LIBRARY_NAME=\"$(notdir $(ARTIFACT_LIB_GST))\"
WRAP_GST_LDFLAGS           := $(call query,LDFLAGS,$(DEPENDENCIES_GST_WRAP))
WRAP_GST_OBJS              := $(call query,OBJ,$(DEPENDENCIES_GST_WRAP))
WRAP_GST_INC               := $(call query,CFLAGS,$(call uniq,$(DEPENDENCIES_GST_WRAP) $(ARTIFACT_ID)))

WRAP_JACK_CFLAGS           := $(VERSION_CFLAGS) -DLSP_PLUGIN_LIBRARY_NAME=\"$(notdir $(ARTIFACT_LIB_JACK))\"
WRAP_JACK_LDFLAGS          := $(call query,LDFLAGS,$(DEPENDENCIES_JACK_WRAP))
WRAP_JACK_OBJS             := $(call query,OBJ,$(DEPENDENCIES_JACK_WRAP))
WRAP_JACK_INC              := $(call query,CFLAGS,$(call uniq,$(DEPENDENCIES_JACK_WRAP) $(ARTIFACT_ID)))

WRAP_VST2_CFLAGS           := $(VERSION_CFLAGS) -DLSP_PLUGIN_LIBRARY_NAME=\"$(notdir $(ARTIFACT_LIB_VST2))\"
WRAP_VST2_LDFLAGS          := $(call query,LDFLAGS,$(DEPENDENCIES_VST2_WRAP))
WRAP_VST2_OBJS             := $(call query,OBJ,$(DEPENDENCIES_VST2_WRAP))
WRAP_VST2_INC              := $(call query,CFLAGS,$(call uniq,$(DEPENDENCIES_VST2_WRAP) $(ARTIFACT_ID)))

OBJ_EXPORT_CFLAGS          := $(VERSION_CFLAGS)
HOST_OBJ_EXPORT_CFLAGS     := $(VERSION_CFLAGS)

#------------------------------------------------------------------------------
# Form lists of XDG files
$(eval \
  $(shell mkdir -p "$(dir $(ARTIFACT_XDG_BATCH_FILE))") \
  $(file >$(ARTIFACT_XDG_LIST_FILE),$(sort $(ARTIFACT_XDG_FILES))) \
)

.DEFAULT_GOAL = all
.PHONY: compile depend dep_clean all install uninstall
.PHONY: jack ladspa dssi lv2 vst2 vst3 clap test meta doc
.PHONY: resources validate
.PHONY: install_jack install_ladspa install_lv2 install_vst2 install_vst3 install_clap install_doc install_xdg
.PHONY: uninstall_jack uninstall_ladspa uninstall_lv2 uninstall_vst2 uninstall_vst3 uninstall_clap uninstall_doc uninstall_xdg
.PHONY: package_jack package_ladspa package_lv2 package_vst2 package_vst3 package_clap package_doc
.PHONY: install_xdg_msg uninstall_xdg_msg
.PHONY: $(DEPENDENCIES) $(DEPENDENCIES_BIN)
.PHONY: $(BIN_INSTALL) $(BIN_UNINSTALL) $(BIN_PACKAGE)
.PHONY: $(ARTIFACT_XDG_INSTALL_DEPS) $(ARTIFACT_XDG_UNINSTALL_DEPS)
.PHONY: $(SRC_DEPS) $(PLUG_DEPS)
.PHONY: $(HOST_SRC_DEPS) $(HOST_PLUG_DEPS)

#------------------------------------------------------------------------------
# Derived source code libraries
$(SRC_DEPS) $(PLUG_DEPS):
	echo "make [$($(@)_NAME)]"
	mkdir -p "$($(@)_BIN)"
	$(MAKE) -C "$($(@)_PATH)" compile VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" NO_DEPENDENCIES="1" DEMO_TEST="0"

#ifeq ($(CROSS_COMPILE),1)
$(HOST_SRC_DEPS) $(HOST_PLUG_DEPS):
	echo "make host [$($(@)_NAME)]"
	mkdir -p "$($(@)_BIN)"
	$(MAKE) -C "$($(@)_PATH)" compile VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" NO_DEPENDENCIES="1" DEMO_TEST="0" BUILD_HOST=1
#endif

$(DEPENDENCIES_BIN):
	echo "make [$($(@)_NAME)]"
	$(MAKE) -C "$($(@)_PATH)" all VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" DEMO_TEST="0"

$(OBJ_PLUG_META) $(OBJ_PLUG_DSP) $(OBJ_PLUG_UI) $(OBJ_PLUG_TEST): $(PLUG_DEPS)

$(HOST_OBJ_PLUG_META) $(HOST_OBJ_PLUG_DSP) $(HOST_OBJ_PLUG_SHARED) $(HOST_OBJ_PLUG_UI) $(HOST_OBJ_PLUG_TEST): $(PLUG_DEPS)

#------------------------------------------------------------------------------
# Compilation
$(OBJ):
	echo "  $(CXX)  [$(LSP_PLUGIN_FW_NAME)] $(CXX_FILE)"
	mkdir -p $(dir $@)
	$(CXX) -o $(@) -c $(CXX_FILE) -fPIC $(CXXFLAGS) $(CXXDEFS) $(LSP_PLUGIN_FW_ALL_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_ALL_CFLAGS) -MMD -MP -MF $(DEP_FILE) -MT $(@)

$(OBJ_WRAP_JACK): EXT_FLAGS=$(WRAP_JACK_CFLAGS)

$(OBJ_WRAP_GST): EXT_FLAGS=$(WRAP_GST_CFLAGS)

$(OBJ_WRAP_LADSPA): EXT_FLAGS=$(WRAP_LADSPA_CFLAGS)

$(OBJ_WRAP_LV2): EXT_FLAGS=$(WRAP_LV2_CFLAGS)

$(OBJ_WRAP_LV2_UI): EXT_FLAGS=$(WRAP_LV2_UI_CFLAGS)

$(OBJ_WRAP_VST2): EXT_FLAGS=$(WRAP_VST2_CFLAGS)

$(OBJ_WRAP_VST3): EXT_FLAGS=$(WRAP_VST3_CFLAGS)

$(OBJ_WRAP_CLAP): EXT_FLAGS=$(WRAP_CLAP_CFLAGS)

$(OBJ_WRAP_CAIRO): EXT_FLAGS=$(WRAP_CAIRO_CFLAGS)

$(OBJ_EXPORT): EXT_FLAGS=$(OBJ_EXPORT_CFLAGS)

ifeq ($(CROSS_COMPILE),1)

$(HOST_OBJ):
	echo "  $(HOST_CXX)  [$(LSP_PLUGIN_FW_NAME)] $(HOST_CXX_FILE)"
	mkdir -p $(dir $@)
	$(HOST_CXX) -o $(@) -c $(HOST_CXX_FILE) -fPIC $(HOST_CXXFLAGS) $(HOST_LSP_PLUGIN_FW_ALL_MFLAGS) $(HOST_EXT_FLAGS) $(INCLUDE) $(HOST_LSP_PLUGIN_FW_ALL_CFLAGS) -MMD -MP -MF $(DEP_FILE) -MT $(@)

$(HOST_OBJ_WRAP_JACK): EXT_FLAGS=$(HOST_WRAP_JACK_CFLAGS)

$(HOST_OBJ_WRAP_GST): EXT_FLAGS=$(HOST_WRAP_GST_CFLAGS)

$(HOST_OBJ_WRAP_LADSPA): EXT_FLAGS=$(HOST_WRAP_LADSPA_CFLAGS)

$(HOST_OBJ_WRAP_LV2): EXT_FLAGS=$(HOST_WRAP_LV2_CFLAGS)

$(HOST_OBJ_WRAP_LV2_UI): EXT_FLAGS=$(HOST_WRAP_LV2_UI_CFLAGS)

$(HOST_OBJ_WRAP_VST2): EXT_FLAGS=$(HOST_WRAP_VST2_CFLAGS)

$(HOST_OBJ_WRAP_VST3): EXT_FLAGS=$(HOST_WRAP_VST3_CFLAGS)

$(HOST_OBJ_WRAP_CLAP): EXT_FLAGS=$(HOST_WRAP_CLAP_CFLAGS)

$(HOST_OBJ_WRAP_CAIRO): EXT_FLAGS=$(HOST_WRAP_CAIRO_CFLAGS)

$(HOST_OBJ_EXPORT): EXT_FLAGS=$(HOST_OBJ_EXPORT_CFLAGS)

endif

#------------------------------------------------------------------------------
# Linking targets
$(LSP_PLUGIN_FW_OBJ_CORE): $(OBJ_CORE)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_CORE))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_CORE) $(LDFLAGS) $(OBJ_CORE)
	
$(LSP_PLUGIN_FW_OBJ_META): $(OBJ_META)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_META))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_META) $(LDFLAGS) $(OBJ_META)

$(LSP_PLUGIN_FW_OBJ_DSP): $(OBJ_DSP)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_DSP))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_DSP) $(LDFLAGS) $(OBJ_DSP)
	
$(LSP_PLUGIN_FW_OBJ_UI): $(OBJ_UI)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_UI))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_UI) $(LDFLAGS) $(OBJ_UI)
	
$(LSP_PLUGIN_FW_OBJ_CTL): $(OBJ_CTL)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_CTL))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_CTL) $(LDFLAGS) $(OBJ_CTL)
	
$(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO): $(OBJ_WRAP_CAIRO)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) $(LDFLAGS) $(OBJ_WRAP_CAIRO)
	
$(LSP_PLUGIN_FW_OBJ_TEST): $(OBJ_TEST)
	echo "  $(LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(LSP_PLUGIN_FW_OBJ_TEST))"
	$(LD) -o $(LSP_PLUGIN_FW_OBJ_TEST) $(LDFLAGS) $(OBJ_TEST)
	
$(LSP_PLUGIN_FW_OBJ_RES): $($(HOST)UTL_RESPACK) resources
	echo "$(notdir $($(HOST)UTL_RESPACK)) $(notdir $(LSP_PLUGIN_FW_SRC_RES))"
	$($(HOST)UTL_RESPACK) -o "$(LSP_PLUGIN_FW_SRC_RES)" -i "$($(HOST)UTL_RES_PATH)" -c "$(LSP_PLUGIN_FW_SRC_RES_CKSUM)" $(LSP_PLUGIN_FW_SRC_RES_EXCL)
	$(CXX) -o $(@) -c $(LSP_PLUGIN_FW_SRC_RES) -fPIC $(CXXFLAGS) $(CXXDEFS) $(LSP_PLUGIN_FW_ALL_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_ALL_CFLAGS) -MMD -MP -MF $(DEP_FILE) -MT $(@)
	
$(LSP_PLUGIN_FW_OBJ_UI_RES): $($(HOST)UTL_RESPACK) resources
	echo "$(notdir $($(HOST)UTL_RESPACK)) $(notdir $(LSP_PLUGIN_FW_SRC_UI_RES))"
	$($(HOST)UTL_RESPACK) -o "$(LSP_PLUGIN_FW_SRC_UI_RES)" -i "$($(HOST)UTL_RES_PATH)" -c "$(LSP_PLUGIN_FW_SRC_UI_RES_CKSUM)" $(LSP_PLUGIN_FW_SRC_UI_RES_EXCL)
	$(CXX) -o $(@) -c $(LSP_PLUGIN_FW_SRC_UI_RES) -fPIC $(CXXFLAGS) $(CXXDEFS) $(LSP_PLUGIN_FW_ALL_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(LSP_PLUGIN_FW_ALL_CFLAGS) -MMD -MP -MF $(DEP_FILE) -MT $(@)

ifeq ($(CROSS_COMPILE),1)

$(HOST_LSP_PLUGIN_FW_OBJ_CORE): $(HOST_OBJ_CORE)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_CORE))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_CORE) $(HOST_LDFLAGS) $(HOST_OBJ_CORE)
	
$(HOST_LSP_PLUGIN_FW_OBJ_META): $(HOST_OBJ_META)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_META))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_META) $(HOST_LDFLAGS) $(HOST_OBJ_META)
	
$(HOST_LSP_PLUGIN_FW_OBJ_DSP): $(HOST_OBJ_DSP)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_DSP))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_DSP) $(HOST_LDFLAGS) $(HOST_OBJ_DSP)
	
$(HOST_LSP_PLUGIN_FW_OBJ_UI): $(HOST_OBJ_UI)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_UI))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_UI) $(HOST_LDFLAGS) $(HOST_OBJ_UI)
	
$(HOST_LSP_PLUGIN_FW_OBJ_CTL): $(HOST_OBJ_CTL)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_CTL))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_CTL) $(HOST_LDFLAGS) $(HOST_OBJ_CTL)
	
$(HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO): $(HOST_OBJ_WRAP_CAIRO)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_WRAP_CAIRO) $(HOST_LDFLAGS) $(HOST_OBJ_WRAP_CAIRO)
	
$(HOST_LSP_PLUGIN_FW_OBJ_TEST): $(HOST_OBJ_TEST)
	echo "  $(HOST_LD)   [$(LSP_PLUGIN_FW_NAME)] $(notdir $(HOST_LSP_PLUGIN_FW_OBJ_TEST))"
	$(HOST_LD) -o $(HOST_LSP_PLUGIN_FW_OBJ_TEST) $(HOST_LDFLAGS) $(HOST_OBJ_TEST)
	
$(HOST_LSP_PLUGIN_FW_OBJ_RES): $($(HOST)UTL_RESPACK) resources
	echo "$(notdir $($(HOST)UTL_RESPACK)) $(notdir $(HOST_LSP_PLUGIN_FW_SRC_RES))"
	$($(HOST)UTL_RESPACK) -o "$(HOST_LSP_PLUGIN_FW_SRC_RES)" -i "$($(HOST)UTL_RES_PATH)" -c "$(HOST_LSP_PLUGIN_FW_SRC_RES_CKSUM)" $(HOST_LSP_PLUGIN_FW_SRC_RES_EXCL)
	$(HOST_CXX) -o $(@) -c $(HOST_LSP_PLUGIN_FW_SRC_RES) -fPIC $(HOST_CXXFLAGS) $(HOST_CXXDEFS) $(HOST_LSP_PLUGIN_FW_ALL_MFLAGS) $(HOST_EXT_FLAGS) $(INCLUDE) $(HOST_LSP_PLUGIN_FW_ALL_CFLAGS) -MMD -MP -MF $(DEP_FILE) -MT $(@)
	
$(HOST_LSP_PLUGIN_FW_OBJ_UI_RES): $($(HOST)UTL_RESPACK) resources
	echo "$(notdir $($(HOST)UTL_RESPACK)) $(notdir $(HOST_LSP_PLUGIN_FW_SRC_UI_RES))"
	$($(HOST)UTL_RESPACK) -o "$(HOST_LSP_PLUGIN_FW_SRC_UI_RES)" -i "$($(HOST)UTL_RES_PATH)" -c "$(HOST_LSP_PLUGIN_FW_SRC_UI_RES_CKSUM)" $(HOST_LSP_PLUGIN_FW_SRC_UI_RES_EXCL)
	$(HOST_CXX) -o $(@) -c $(HOST_LSP_PLUGIN_FW_SRC_UI_RES) -fPIC $(HOST_CXXFLAGS) $(HOST_CXXDEFS) $(HOST_LSP_PLUGIN_FW_ALL_MFLAGS) $(HOST_EXT_FLAGS) $(INCLUDE) $(HOST_LSP_PLUGIN_FW_ALL_CFLAGS) -MMD -MP -MF $(DEP_FILE) -MT $(@)

endif

#------------------------------------------------------------------------------
# Utilities
$($(HOST)UTL_VALIDATOR): $($(HOST)UTL_VALIDATOR_DEPS) $($(HOST)UTL_VALIDATOR_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $($(HOST)CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_VALIDATOR_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_VALIDATOR_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_VALIDATOR_LDFLAGS)

$($(HOST)UTL_JACK_MAKE): $($(HOST)UTL_JACK_MAKE_DEPS) $($(HOST)UTL_JACK_MAKE_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $($(HOST)CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_JACK_MAKE_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_JACK_MAKE_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_JACK_MAKE_LDFLAGS)

$($(HOST)UTL_GST_MAKE): $($(HOST)UTL_GST_MAKE_DEPS) $($(HOST)UTL_GST_MAKE_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $($(HOST)CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_GST_MAKE_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_GST_MAKE_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_GST_MAKE_LDFLAGS)

$($(HOST)UTL_VST2_MAKE): $($(HOST)UTL_VST2_MAKE_DEPS) $($(HOST)UTL_VST2_MAKE_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $($(HOST)CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_VST2_MAKE_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_VST2_MAKE_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_VST2_MAKE_LDFLAGS)

$($(HOST)UTL_VST3_MODINFO): $($(HOST)UTL_VST3_MODINFO_DEPS) $($(HOST)UTL_VST3_MODINFO_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $($(HOST)CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_VST3_MODINFO_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_VST3_MODINFO_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_VST3_MODINFO_LDFLAGS)

$($(HOST)UTL_LV2TTL_GEN): $($(HOST)UTL_LV2TTL_GEN_DEPS) $($(HOST)UTL_LV2TTL_GEN_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $($(HOST)CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_LV2TTL_GEN_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_LV2TTL_GEN_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_LV2TTL_GEN_LDFLAGS)

$($(HOST)UTL_REPOSITORY): $($(HOST)UTL_REPOSITORY_DEPS) $($(HOST)UTL_REPOSITORY_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_REPOSITORY_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_REPOSITORY_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_REPOSITORY_LDFLAGS)
	echo "utl_repository OK"

$($(HOST)UTL_RESPACK): $($(HOST)UTL_RESPACK_DEPS) $($(HOST)UTL_RESPACK_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_RESPACK_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_RESPACK_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_RESPACK_LDFLAGS)
	echo "utl_respack OK"
	
$($(HOST)UTL_PLUGLIST_GEN): $($(HOST)UTL_PLUGLIST_GEN_DEPS) $($(HOST)UTL_PLUGLIST_GEN_OBJS) $($(HOST)PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(@))"
	mkdir -p $(dir $(@))
	$($(HOST)CXX) -o $(@) $($(HOST)UTL_PLUGLIST_GEN_OBJS) $($(HOST)CXXFLAGS) $($(HOST)CXXDEFS) $($(HOST)UTL_PLUGLIST_GEN_LIBS) $($(HOST)EXE_FLAGS) $($(HOST)UTL_PLUGLIST_GEN_LDFLAGS)

#------------------------------------------------------------------------------
# Binaries
$(ARTIFACT_LIB_CLAP): $(OBJ_WRAP_CLAP) $(ARTIFACT_LIB_CLAP_DEPS) $(ARTIFACT_LIB_CLAP_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_CLAP))"
	$(CXX) -o $(ARTIFACT_LIB_CLAP) $(ARTIFACT_LIB_CLAP_LIBS) $(ARTIFACT_LIB_CLAP_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_CLAP_LDFLAGS)

$(ARTIFACT_LIB_GST): $(OBJ_WRAP_GST) $(ARTIFACT_LIB_GST_DEPS) $(ARTIFACT_LIB_GST_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_GST))"
	$(CXX) -o $(ARTIFACT_LIB_GST) $(ARTIFACT_LIB_GST_LIBS) $(ARTIFACT_LIB_GST_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_GST_LDFLAGS)
	
$(ARTIFACT_LIB_JACK): $(OBJ_WRAP_JACK) $(ARTIFACT_LIB_JACK_DEPS) $(ARTIFACT_LIB_JACK_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_JACK))"
	$(CXX) -o $(ARTIFACT_LIB_JACK) $(ARTIFACT_LIB_JACK_LIBS) $(ARTIFACT_LIB_JACK_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_JACK_LDFLAGS)
	
$(ARTIFACT_LIB_LADSPA): $(OBJ_WRAP_LADSPA) $(ARTIFACT_LIB_LADSPA_DEPS) $(ARTIFACT_LIB_LADSPA_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_LADSPA))"
	$(CXX) -o $(ARTIFACT_LIB_LADSPA) $(ARTIFACT_LIB_LADSPA_LIBS) $(ARTIFACT_LIB_LADSPA_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_LADSPA_LDFLAGS)
	
$(ARTIFACT_LIB_LV2): $(OBJ_WRAP_LV2) $(ARTIFACT_LIB_LV2_DEPS) $(ARTIFACT_LIB_LV2_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_LV2))"
	$(CXX) -o $(ARTIFACT_LIB_LV2) $(ARTIFACT_LIB_LV2_LIBS) $(ARTIFACT_LIB_LV2_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_LV2_LDFLAGS)
	
$(ARTIFACT_LIB_LV2_UI): $(OBJ_WRAP_LV2_UI) $(ARTIFACT_LIB_LV2_UI_DEPS) $(ARTIFACT_LIB_LV2_UI_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_LV2_UI))"
	$(CXX) -o $(ARTIFACT_LIB_LV2_UI) $(ARTIFACT_LIB_LV2_UI_LIBS) $(ARTIFACT_LIB_LV2_UI_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_LV2_UI_LDFLAGS)
	
$(ARTIFACT_LIB_VST2): $(OBJ_WRAP_VST2) $(ARTIFACT_LIB_VST2_DEPS) $(ARTIFACT_LIB_VST2_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_VST2))"
	$(CXX) -o $(ARTIFACT_LIB_VST2) $(ARTIFACT_LIB_VST2_LIBS) $(ARTIFACT_LIB_VST2_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_VST2_LDFLAGS)
	
$(ARTIFACT_LIB_VST3): $(OBJ_WRAP_VST3) $(ARTIFACT_LIB_VST3_DEPS) $(ARTIFACT_LIB_VST3_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB_VST3))"
	$(CXX) -o $(ARTIFACT_LIB_VST3) $(ARTIFACT_LIB_VST3_LIBS) $(ARTIFACT_LIB_VST3_OBJS) $(SO_FLAGS) $(ARTIFACT_LIB_VST3_LDFLAGS)
	
$(ARTIFACT_BIN_TEST): $(ARTIFACT_BIN_TEST_DEPS) $(ARTIFACT_BIN_TEST_OBJS) $(PLUG_DEPS)
	echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_BIN_TEST))"
	$(CXX) -o $(ARTIFACT_BIN_TEST) $(ARTIFACT_BIN_TEST_LIBS) $(ARTIFACT_BIN_TEST_OBJS) $(EXE_FLAGS) $(ARTIFACT_BIN_TEST_LDFLAGS)

# All targets
validate: $($(HOST)UTL_VALIDATOR)
	echo "Validating plugin metadata"
	$($(HOST)UTL_VALIDATOR)

resources: $($(HOST)UTL_REPOSITORY)
	echo "  $($(HOST)UTL_REPOSITORY)"
	$($(HOST)UTL_REPOSITORY) -o "$($(HOST)UTL_RES_PATH)" $(LSP_PLUGIN_FW_OBJ_VARS) -m "$(ROOTDIR)/res/manifest.json" "$(ROOTDIR)" "$(call pathconv,$(ROOTDIR))/modules/*"

all: $(BUILD_TARGETS)

clap: $(ARTIFACT_LIB_CLAP) $(DEPENDENCIES_BIN) $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_UI_BIN))

gst: resources $($(HOST)UTL_GST_MAKE) $(ARTIFACT_LIB_GST) $(DEPENDENCIES_BIN)
	echo "  $(notdir $($(HOST)UTL_GST_MAKE)) [$(ARTIFACT_NAME)] $(patsubst $(LSP_PLUGIN_FW_BIN)/%,%, $(ARTIFACT_LIB_GST_PATH))"
	mkdir -p "$(ARTIFACT_LIB_GST_PATH)"
	$($(HOST)UTL_GST_MAKE) -o "$(ARTIFACT_LIB_GST_PATH)" -m "$($(HOST)UTL_RES_PATH)/manifest.json" -t "$(LSP_PLUGIN_FW_INC)/lsp-plug.in/plug-fw/wrap/gstreamer/template/plugin.h"
	echo "  make GST"
	echo "" > $(ARTIFACT_LIB_GST_SETTINGS)
	echo '# Auto-generated script, do not edit' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_ARTIFACT_NAME=$(PLUGIN_SHARED_NAME)' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_ARTIFACT_GROUP=$(PLUGIN_GROUP_NAME)' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_LIBRARY_NAME=$(notdir $(ARTIFACT_LIB_GST))' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_PREFIX=' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_OBJS=$(WRAP_GST_OBJS)' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_CFLAGS=$(WRAP_GST_CFLAGS)' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_CXXFLAGS=$(WRAP_GST_CFLAGS)' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_LDFLAGS=$(WRAP_GST_LDFLAGS)' >> $(ARTIFACT_LIB_GST_SETTINGS)
	echo 'EXT_INCLUDE=$(WRAP_GST_INC)' >> $(ARTIFACT_LIB_GST_SETTINGS)
	$(MAKE) -C "$(ARTIFACT_LIB_GST_PATH)" depend VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_GST_SETTINGS)"
	$(MAKE) -C "$(ARTIFACT_LIB_GST_PATH)"  VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_GST_SETTINGS)"

doc: meta
	echo "  make DOC"
	mkdir -p $(ARTIFACT_DOC_BLD_PATH)
	mkdir -p $(ARTIFACT_DOC_PATH)
	cp -R $(ARTIFACT_DOC_SRC_ALL) $(ARTIFACT_DOC_BLD_PATH)/
	cp "$(ROOTDIR)/README.md" $(ARTIFACT_DOC_BLD_PATH)/
	cp $(wildcard $(ROOTDIR)/COPYING*) $(wildcard $(ROOTDIR)/LICENSE*) $(ARTIFACT_DOC_BLD_PATH)/
	cp -R "$(LSP_PLUGIN_FW_BIN)/plugins.php" $(ARTIFACT_DOC_BLD_PATH)/config
	echo $(PHP) makefile.php
	$(PHP) $(ARTIFACT_DOC_BLD_PATH)/makefile.php "$(ARTIFACT_DOC_BLD_PATH)" > $(ARTIFACT_DOC_BLD_PATH)/Makefile
	$(if $(ARTIFACT_DOC_SRC_RES),cp -R $(ARTIFACT_DOC_SRC_RES) $(ARTIFACT_DOC_PATH)/)
	mkdir -p $(ARTIFACT_DOC_PATH)/img/plugins
	$(if $(ARTIFACT_DOC_PLUG_RES),cp -R $(ARTIFACT_DOC_PLUG_RES) $(ARTIFACT_DOC_PATH)/img/plugins)
	$(MAKE) -C "$(ARTIFACT_DOC_BLD_PATH)" PHP=$(PHP) HTMLDIR=$(ARTIFACT_DOC_PATH) VERBOSE=$(VERBOSE)

jack: $($(HOST)UTL_JACK_MAKE) $(ARTIFACT_LIB_JACK) $(DEPENDENCIES_BIN) $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_UI_BIN))
	echo "  $(notdir $($(HOST)UTL_JACK_MAKE)) [$(ARTIFACT_NAME)] $(patsubst $(LSP_PLUGIN_FW_BIN)/%,%, $(ARTIFACT_LIB_JACK_PATH))"
	mkdir -p "$(ARTIFACT_LIB_JACK_PATH)"
	$($(HOST)UTL_JACK_MAKE) "$(ARTIFACT_LIB_JACK_PATH)"
	echo "  make JACK"
	echo "" > $(ARTIFACT_LIB_JACK_SETTINGS)
	echo '# Auto-generated script, do not edit' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	echo 'EXT_ARTIFACT_NAME=$(PLUGIN_SHARED_NAME)' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	echo 'EXT_ARTIFACT_GROUP=$(PLUGIN_GROUP_NAME)' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	echo 'EXT_PREFIX=$(PLUGIN_ARTIFACT_PREFIX)' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	echo 'EXT_OBJS=$(WRAP_JACK_OBJS)' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	echo 'EXT_CFLAGS=$(WRAP_JACK_CFLAGS)' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	echo 'EXT_CXXFLAGS=$(WRAP_JACK_CFLAGS)' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	echo 'EXT_LDFLAGS=$(WRAP_JACK_LDFLAGS)' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	echo 'EXT_INCLUDE=$(WRAP_JACK_INC)' >> $(ARTIFACT_LIB_JACK_SETTINGS)
	$(MAKE) -C "$(ARTIFACT_LIB_JACK_PATH)" depend VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"
	$(MAKE) -C "$(ARTIFACT_LIB_JACK_PATH)"  VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"

ladspa: $(ARTIFACT_LIB_LADSPA) $(DEPENDENCIES_BIN)

meta: $($(HOST)UTL_PLUGLIST_GEN)
	mkdir -p "$(LSP_PLUGIN_FW_BIN)"
	$($(HOST)UTL_PLUGLIST_GEN) --php "$(LSP_PLUGIN_FW_BIN)/plugins.php" --json "$(LSP_PLUGIN_FW_BIN)/plugins.json"

lv2: $($(HOST)UTL_LV2TTL_GEN) $(ARTIFACT_LIB_LV2) $(DEPENDENCIES_BIN) $(call fcheck,ui,$(ENABLED_FEATURES),$(ARTIFACT_LIB_LV2_UI) $(DEPENDENCIES_UI_BIN)) 

vst2: $($(HOST)UTL_VST2_MAKE) $(ARTIFACT_LIB_VST2) $(DEPENDENCIES_BIN) $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_UI_BIN))
	echo "  $(notdir $($(HOST)UTL_VST2_MAKE)) [$(ARTIFACT_NAME)] $(patsubst $(LSP_PLUGIN_FW_BIN)/%,%, $(ARTIFACT_LIB_VST2_PATH))"
	mkdir -p "$(ARTIFACT_LIB_VST2_PATH)"
	$($(HOST)UTL_VST2_MAKE) "$(ARTIFACT_LIB_VST2_PATH)"
	echo "  make VST2"
	echo "" > $(ARTIFACT_LIB_VST2_SETTINGS)
	echo '# Auto-generated script, do not edit' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	echo 'EXT_ARTIFACT_NAME=$(PLUGIN_SHARED_NAME)' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	echo 'EXT_ARTIFACT_GROUP=$(PLUGIN_GROUP_NAME)' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	echo 'EXT_PREFIX=$(PLUGIN_ARTIFACT_PREFIX)' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	echo 'EXT_OBJS=$(WRAP_VST2_OBJS)' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	echo 'EXT_CFLAGS=$(WRAP_VST2_CFLAGS)' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	echo 'EXT_CXXFLAGS=$(WRAP_VST2_CFLAGS)' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	echo 'EXT_LDFLAGS=$(WRAP_VST2_LDFLAGS)' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	echo 'EXT_INCLUDE=$(WRAP_VST2_INC)' >> $(ARTIFACT_LIB_VST2_SETTINGS)
	$(MAKE) -C "$(ARTIFACT_LIB_VST2_PATH)" depend VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_VST2_SETTINGS)"
	$(MAKE) -C "$(ARTIFACT_LIB_VST2_PATH)"  VERBOSE="$(VERBOSE)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_VST2_SETTINGS)"

vst3: $($(HOST)UTL_VST3_MODINFO) $(ARTIFACT_LIB_VST3) $(DEPENDENCIES_BIN) $(call fcheck,ui,$(ENABLED_FEATURES),$(DEPENDENCIES_UI_BIN))

xdg:

test: $(ARTIFACT_BIN_TEST)

#------------------------------------------------------------------------------
# Installation
$(BIN_INSTALL): $(DEPENDENCIES_BIN)
	echo "Install $(notdir $($(BIN_INST_ARTIFACT_ID)_PATH))"
	$(MAKE) -C "$($(BIN_INST_ARTIFACT_ID)_PATH)" install DESTDIR="$(DESTDIR)"

$(ARTIFACT_XDG_INST): install_xdg_msg
	$(MAKE) -C "$($(ARTIFACT_XDG_INST_ID)_PATH)/res/xdg" install_xdg DESTDIR="$(DESTDIR)" FILE_LIST="$(ARTIFACT_XDG_LIST_FILE)" BATCH_FILE="$(ARTIFACT_XDG_BATCH_FILE)" CONFIG=$(ROOTDIR)/.config.mk

install: validate $(BIN_INSTALL) $(INSTALL_TARGETS)
	echo "Install OK"

install_clap: clap
	echo "Install clap"
	mkdir -p "$(DESTDIR)$(CLAP_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_CLAP) "$(DESTDIR)$(CLAP_LIBDIR)/"

install_doc: doc
	echo "Install doc"
	mkdir -p "$(DESTDIR)$(DOC_SHAREDDIR)"
	cp -R $(ARTIFACT_DOC_PATH)/* "$(DESTDIR)$(DOC_SHAREDDIR)"

install_gst: gst
	echo "Install gst"
	mkdir -p "$(DESTDIR)$(GSTREAMER_INSTDIR)"
	mkdir -p "$(DESTDIR)$(GST_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_GST) "$(DESTDIR)$(GST_LIBDIR)/"
	$(MAKE) -C "$(ARTIFACT_LIB_GST_PATH)" install DESTDIR="$(DESTDIR)$(GSTREAMER_INSTDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_GST_SETTINGS)"

install_jack: jack
	echo "Install jack"
	mkdir -p "$(DESTDIR)$(JACK_LIBDIR)"
	mkdir -p "$(DESTDIR)$(BINDIR)"
	$(INSTALL) $(ARTIFACT_LIB_JACK) "$(DESTDIR)$(JACK_LIBDIR)/"
	$(MAKE) -C "$(ARTIFACT_LIB_JACK_PATH)" install DESTDIR="$(DESTDIR)$(BINDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"

install_ladspa: ladspa
	echo "Install ladspa"
	mkdir -p "$(DESTDIR)$(LADSPA_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_LADSPA) "$(DESTDIR)$(LADSPA_LIBDIR)/"

install_lv2: lv2
	echo "Install lv2"
	mkdir -p "$(DESTDIR)$(LV2_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_LV2) "$(DESTDIR)$(LV2_LIBDIR)/"
	$(call fcheck,ui,$(ENABLED_FEATURES), \
	  $(INSTALL) $(ARTIFACT_LIB_LV2_UI) "$(DESTDIR)$(LV2_LIBDIR)/" \
	)
	$($(HOST)UTL_LV2TTL_GEN) -o "$(DESTDIR)$(LV2_LIBDIR)" -i "$(ARTIFACT_LIB_LV2)" $(call fcheck,ui,$(ENABLED_FEATURES),-ui $(ARTIFACT_LIB_LV2_UI))

install_vst2: vst2
	echo "Install vst2"
	mkdir -p "$(DESTDIR)$(VST2_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_VST2) "$(DESTDIR)$(VST2_LIBDIR)/"
	$(MAKE) -C "$(ARTIFACT_LIB_VST2_PATH)" install DESTDIR="$(DESTDIR)$(VST2_LIBDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_VST2_SETTINGS)"

install_vst3: vst3
	echo "Install vst3"
	mkdir -p "$(DESTDIR)$(VST3_LIBDIR)"
	mkdir -p "$(DESTDIR)$(VST3_LIBDIR)/Contents/Resources"
	mkdir -p "$(DESTDIR)$(VST3_LIBDIR)/Contents/Resources"
	mkdir -p "$(DESTDIR)$(VST3_LIBDIR)/Contents/${VST3_ARCH}"
	$($(HOST)UTL_VST3_MODINFO) "$(DESTDIR)$(VST3_LIBDIR)/Contents/Resources/moduleinfo.json"
	$(INSTALL) $(ARTIFACT_LIB_VST3) "$(DESTDIR)$(VST3_LIBDIR)/Contents/${VST3_ARCH}/"

install_xdg_msg:
	echo "Install xdg"

install_xdg: install_xdg_msg $(ARTIFACT_XDG_INST)

#------------------------------------------------------------------------------
# Packaging
$(BIN_PACKAGE): $(DEPENDENCIES_BIN)

package: $(PACKAGE_TARGETS)
	echo "Package OK"

package_clap: clap $(BIN_PACKAGE)
	echo "Package clap"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(CLAP_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_CLAP) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(CLAP_LIBDIR)/"
	$(foreach target,$(BIN_PACKAGE_CLAP), \
	    $(MAKE) -C "$($(patsubst package_%,%,$(target))_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)"; \
	)
	$(foreach file,$(BIN_EXTRA_FILES), \
	    cp -f $(file) "$(BUILDDIR)/$(BIN_PKG_DIR)/"; \
	)
	cp -f $(BIN_PKG_FILES) "$(BUILDDIR)/$(BIN_PKG_DIR)/"
	tar -C "$(BUILDDIR)" -czf "$(BUILDDIR)/$(BIN_PKG_DIR).tar.gz" "$(BIN_PKG_DIR)"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"

package_doc: doc
	echo "Package doc"
	-rm -rf "$(BUILDDIR)/$(ARTIFACT_DOC_PKG_DIR)"
	mkdir -p "$(BUILDDIR)/$(ARTIFACT_DOC_PKG_DIR)/$(DOC_SHAREDDIR)"
	cp -R $(ARTIFACT_DOC_PATH)/* "$(BUILDDIR)/$(ARTIFACT_DOC_PKG_DIR)/$(DOC_SHAREDDIR)"
	$(foreach file,$(BIN_EXTRA_FILES), \
	    cp -f $(file) "$(BUILDDIR)/$(ARTIFACT_DOC_PKG_DIR)/"; \
	)
	cp -f $(BIN_PKG_FILES) "$(BUILDDIR)/$(ARTIFACT_DOC_PKG_DIR)/"
	tar -C "$(BUILDDIR)" -czf "$(BUILDDIR)/$(ARTIFACT_DOC_PKG_DIR).tar.gz" "$(ARTIFACT_DOC_PKG_DIR)"
	-rm -rf "$(BUILDDIR)/$(ARTIFACT_DOC_PKG_DIR)"

package_gst: gst $(BIN_PACKAGE)
	echo "Package gst"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(GSTREAMER_PKGDIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(GST_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_GST) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(GST_LIBDIR)/"
	$(MAKE) -C "$(ARTIFACT_LIB_GST_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)/$(GSTREAMER_PKGDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_GST_SETTINGS)"
	$(foreach target,$(BIN_PACKAGE_GST), \
	    $(MAKE) -C "$($(patsubst package_%,%,$(target))_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)"; \
	)
	$(foreach file,$(BIN_EXTRA_FILES), \
	    cp -f $(file) "$(BUILDDIR)/$(BIN_PKG_DIR)/"; \
	)
	cp -f $(BIN_PKG_FILES) "$(BUILDDIR)/$(BIN_PKG_DIR)/"
	tar -C "$(BUILDDIR)" -czf "$(BUILDDIR)/$(BIN_PKG_DIR).tar.gz" "$(BIN_PKG_DIR)"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"

package_jack: jack $(BIN_PACKAGE)
	echo "Package jack"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(JACK_LIBDIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(BINDIR)"
	$(INSTALL) $(ARTIFACT_LIB_JACK) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(JACK_LIBDIR)/"
	$(MAKE) -C "$(ARTIFACT_LIB_JACK_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)/$(BINDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"
	$(foreach target,$(BIN_PACKAGE_JACK), \
	    $(MAKE) -C "$($(patsubst package_%,%,$(target))_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)"; \
	)
	$(if $(findstring xdg,$(ENABLED_FEATURES)), \
	    $(foreach target,$(ARTIFACT_XDG_INST), \
	        $(MAKE) -C "$($(patsubst xdg_install_%,%,$(target))_PATH)/res/xdg" \
	            install_xdg DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)" \
	            FILE_LIST="$(ARTIFACT_XDG_LIST_FILE)" \
	            BATCH_FILE="$(ARTIFACT_XDG_BATCH_FILE)"; \
	    ) \
	)
	$(foreach file,$(BIN_EXTRA_FILES), \
	    cp -f $(file) "$(BUILDDIR)/$(BIN_PKG_DIR)/"; \
	)
	cp -f $(BIN_PKG_FILES) "$(BUILDDIR)/$(BIN_PKG_DIR)/"
	tar -C "$(BUILDDIR)" -czf "$(BUILDDIR)/$(BIN_PKG_DIR).tar.gz" "$(BIN_PKG_DIR)"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"

package_ladspa: ladspa $(BIN_PACKAGE)
	echo "Package ladspa"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(LADSPA_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_LADSPA) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(LADSPA_LIBDIR)/"
	$(foreach target,$(BIN_PACKAGE_LADSPA), \
	    $(MAKE) -C "$($(patsubst package_%,%,$(target))_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)"; \
	)
	$(foreach file,$(BIN_EXTRA_FILES), \
	    cp -f $(file) "$(BUILDDIR)/$(BIN_PKG_DIR)/"; \
	)
	cp -f $(BIN_PKG_FILES) "$(BUILDDIR)/$(BIN_PKG_DIR)/"
	tar -C "$(BUILDDIR)" -czf "$(BUILDDIR)/$(BIN_PKG_DIR).tar.gz" "$(BIN_PKG_DIR)"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"

package_lv2: lv2 $(BIN_PACKAGE)
	echo "Package lv2"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(LV2_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_LV2) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(LV2_LIBDIR)/"
	$(call fcheck,ui,$(ENABLED_FEATURES), \
	  $(INSTALL) $(ARTIFACT_LIB_LV2_UI) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(LV2_LIBDIR)/" \
	)
	$($(HOST)UTL_LV2TTL_GEN) -o "$(BUILDDIR)/$(BIN_PKG_DIR)/$(LV2_LIBDIR)" -i "$(ARTIFACT_LIB_LV2)" $(call fcheck,ui,$(ENABLED_FEATURES),-ui $(ARTIFACT_LIB_LV2_UI))
	$(foreach target,$(BIN_PACKAGE_LV2), \
	    $(MAKE) -C "$($(patsubst package_%,%,$(target))_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)"; \
	)
	$(foreach file,$(BIN_EXTRA_FILES), \
	    cp -f $(file) "$(BUILDDIR)/$(BIN_PKG_DIR)/"; \
	)
	cp -f $(BIN_PKG_FILES) "$(BUILDDIR)/$(BIN_PKG_DIR)/"
	tar -C "$(BUILDDIR)" -czf "$(BUILDDIR)/$(BIN_PKG_DIR).tar.gz" "$(BIN_PKG_DIR)"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"

package_vst2: vst2 $(BIN_PACKAGE)
	echo "Package vst2"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(VST2_LIBDIR)"
	$(INSTALL) $(ARTIFACT_LIB_VST2) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(VST2_LIBDIR)/"
	$(MAKE) -C "$(ARTIFACT_LIB_VST2_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)/$(VST2_LIBDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_VST2_SETTINGS)"
	$(foreach target,$(BIN_PACKAGE_VST2), \
	    $(MAKE) -C "$($(patsubst package_%,%,$(target))_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)"; \
	)
	$(foreach file,$(BIN_EXTRA_FILES), \
	    cp -f $(file) "$(BUILDDIR)/$(BIN_PKG_DIR)/"; \
	)
	cp -f $(BIN_PKG_FILES) "$(BUILDDIR)/$(BIN_PKG_DIR)/"
	tar -C "$(BUILDDIR)" -czf "$(BUILDDIR)/$(BIN_PKG_DIR).tar.gz" "$(BIN_PKG_DIR)"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"

package_vst3: vst3 $(BIN_PACKAGE)
	echo "Package vst3"
	-rm -rf  "$(BUILDDIR)/$(BIN_PKG_DIR)"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(VST3_LIBDIR)/Contents/Resources"
	mkdir -p "$(BUILDDIR)/$(BIN_PKG_DIR)/$(VST3_LIBDIR)/Contents/${VST3_ARCH}"
	$($(HOST)UTL_VST3_MODINFO) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(VST3_LIBDIR)/Contents/Resources/moduleinfo.json"
	$(INSTALL) $(ARTIFACT_LIB_VST3) "$(BUILDDIR)/$(BIN_PKG_DIR)/$(VST3_LIBDIR)/Contents/${VST3_ARCH}/"
	$(foreach target,$(BIN_PACKAGE_VST3), \
	    $(MAKE) -C "$($(patsubst package_%,%,$(target))_PATH)" install DESTDIR="$(BUILDDIR)/$(BIN_PKG_DIR)"; \
	)
	$(foreach file,$(BIN_EXTRA_FILES), \
	    cp -f $(file) "$(BUILDDIR)/$(BIN_PKG_DIR)/"; \
	)
	cp -f $(BIN_PKG_FILES) "$(BUILDDIR)/$(BIN_PKG_DIR)/"
	tar -C "$(BUILDDIR)" -czf "$(BUILDDIR)/$(BIN_PKG_DIR).tar.gz" "$(BIN_PKG_DIR)"
	-rm -rf "$(BUILDDIR)/$(BIN_PKG_DIR)"

#------------------------------------------------------------------------------
# Deinstallation
$(BIN_UNINSTALL):
	echo "Uninstall $(notdir $($(BIN_UNINST_ARTIFACT_ID)_PATH))"
	$(MAKE) -C "$($(BIN_UNINST_ARTIFACT_ID)_PATH)" uninstall DESTDIR="$(DESTDIR)"

$(ARTIFACT_XDG_UNINST): uninstall_xdg_msg
	mkdir -p $(LSP_PLUGIN_FW_BIN)/xdg
	$(file >$(LSP_PLUGIN_FW_BIN)/xdg/$(ARTIFACT_XDG_UNINST_ID).list,$(sort $(ARTIFACT_XDG_FILES)))
	$(MAKE) -C "$($(ARTIFACT_XDG_UNINST_ID)_PATH)/res/xdg" uninstall_xdg DESTDIR="$(DESTDIR)" FILE_LIST="$(LSP_PLUGIN_FW_BIN)/xdg/$(ARTIFACT_XDG_UNINST_ID).list" BATCH_FILE="$(LSP_PLUGIN_FW_BIN)/xdg/$(ARTIFACT_XDG_UNINST_ID).mk" CONFIG=$(ROOTDIR)/.config.mk

uninstall: $(BIN_UNINSTALL) $(UNINSTALL_TARGETS)
	@echo "Uninstall OK"

uninstall_clap:
	echo "Uninstall clap"
	-rm -f "$(DESTDIR)$(CLAP_LIBDIR)/$(notdir $(ARTIFACT_LIB_CLAP))"
	
uninstall_doc:
	echo "Uninstall doc"
	-rm -rf "$(DESTDIR)$(DOC_SHAREDDIR)"

uninstall_gst:
	echo "Uninstall gst"
	-rm -f $(INSTALL) $(DESTDIR)$(GST_LIBDIR)/$(notdir $(ARTIFACT_LIB_GST))
	-rmdir "$(DESTDIR)$(GST_LIBDIR)" >/dev/null 2>&1
	$(MAKE) -C "$(ARTIFACT_LIB_GST_PATH)" uninstall DESTDIR="$(DESTDIR)$(GSTREAMER_INSTDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_GST_SETTINGS)"

uninstall_jack:
	echo "Uninstall jack"
	-rm -f $(INSTALL) $(DESTDIR)$(JACK_LIBDIR)/$(notdir $(ARTIFACT_LIB_JACK))
	-rmdir "$(DESTDIR)$(JACK_LIBDIR)" >/dev/null 2>&1
	$(MAKE) -C "$(ARTIFACT_LIB_JACK_PATH)" uninstall DESTDIR="$(DESTDIR)$(BINDIR)" CONFIG="$(CONFIG)" SETTINGS="$(ARTIFACT_LIB_JACK_SETTINGS)"

uninstall_ladspa:
	echo "Uninstall ladspa"
	-rm -f "$(DESTDIR)$(LADSPA_LIBDIR)/$(notdir $(ARTIFACT_LIB_LADSPA))"

uninstall_lv2:
	echo "Uninstall lv2"
	mkdir -p "$(DESTDIR)$(LV2_LIBDIR)"
	-rm -rf "$(DESTDIR)$(LV2_LIBDIR)"

uninstall_vst2:
	echo "Uninstall vst2"
	-rm -rf "$(DESTDIR)$(VST2_LIBDIR)"

uninstall_vst3:
	echo "Uninstall vst3"
	-rm -rf "$(DESTDIR)$(VST3_LIBDIR)"

uninstall_xdg_msg:
	echo "Unnstall xdg"

uninstall_xdg: uninstall_xdg_msg $(ARTIFACT_XDG_UNINST)

# Dependencies
-include $(foreach objfile,$(OBJ) $(HOST_OBJ),$(patsubst %.o,%.d,$(objfile)))
