#
# Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#------------------------------------------------------------------------------
# Features enabled by default
DEFAULT_FEATURES = doc ladspa lv2 vst2

#------------------------------------------------------------------------------
# Plugin dependencies
DEPENDENCIES_COMMON = \
  LSP_COMMON_LIB \
  LSP_DSP_LIB \
  LSP_DSP_UNITS \
  LSP_LLTL_LIB \
  LSP_RUNTIME_LIB \
  LSP_PLUGIN_FW \
  LSP_3RD_PARTY

DEPENDENCIES_BIN =

DEPENDENCIES_COMMON_UI = \
  LSP_R3D_BASE_LIB \
  LSP_R3D_IFACE \
  LSP_TK_LIB \
  LSP_WS_LIB

TEST_DEPENDENCIES = \
  LSP_TEST_FW
  
TEST_DEPENDENCIES_UI = \
  LSP_TEST_FW

#------------------------------------------------------------------------------
# Platform-specific dependencies
ifeq ($(PLATFORM),Linux)
  DEPENDENCIES_BIN += \
    LSP_R3D_GLX_LIB

  TEST_DEPENDENCIES_UI += \
    LSP_R3D_GLX_LIB \
    LSP_R3D_COMMON_LIB \
    LIBGL
endif

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES_BIN += \
    LSP_R3D_GLX_LIB

  TEST_DEPENDENCIES_UI += \
    LSP_R3D_GLX_LIB \
    LSP_R3D_COMMON_LIB \
    LIBGL
    
  DEPENDENCIES_COMMON += \
    LIBICONV
endif

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES_BIN += \
    LSP_R3D_WGL_LIB

  DEPENDENCIES_COMMON += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM \
    LIBMPR \
    LIBGDI32 \
    LIBD2D1 \
    LIBOLE \
    LIBWINCODEC \
    LIBDWRITE \
    LIBUUID
endif

#------------------------------------------------------------------------------
# Jack build dependencies
DEPENDENCIES_JACK = \
  $(DEPENDENCIES_COMMON)

DEPENDENCIES_JACK_UI = \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_JACK_WRAP = \
  LIBDL \
  LSP_COMMON_LIB

ifeq ($(PLATFORM),Linux)
  DEPENDENCIES_JACK += \
    LIBPTHREAD \
    LIBDL \
    LIBJACK \
    LIBSNDFILE

  DEPENDENCIES_JACK_UI += \
    LIBPTHREAD \
    LIBDL \
    LIBJACK \
    LIBSNDFILE \
    LIBX11 \
    LIBXRANDR \
    LIBCAIRO \
    LIBFREETYPE
endif

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES_JACK += \
    LIBPTHREAD \
    LIBDL \
    LIBJACK \
    LIBSNDFILE
    
  DEPENDENCIES_JACK_UI += \
    LIBDL \
    LIBJACK \
    LIBSNDFILE \
    LIBX11 \
    LIBXRANDR \
    LIBCAIRO \
    LIBFREETYPE
endif

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES_JACK += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM \
    LIBMPR \
    LIBGDI32 \
    LIBD2D1 \
    LIBOLE \
    LIBWINCODEC \
    LIBDWRITE \
    LIBUUID
endif

#------------------------------------------------------------------------------
# LADSPA build dependencies
DEPENDENCIES_LADSPA = \
  $(DEPENDENCIES_COMMON)

ifeq ($(PLATFORM),Linux)
  DEPENDENCIES_LADSPA += \
    LIBPTHREAD \
    LIBDL \
    LIBLADSPA \
    LIBSNDFILE
endif

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES_LADSPA += \
    LIBPTHREAD \
    LIBDL \
    LIBLADSPA \
    LIBSNDFILE
endif

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES_LADSPA += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM \
    LIBMPR \
    LIBGDI32 \
    LIBD2D1 \
    LIBOLE \
    LIBWINCODEC \
    LIBDWRITE \
    LIBUUID
endif

#------------------------------------------------------------------------------
# LV2 build dependencies
DEPENDENCIES_LV2 = \
  $(DEPENDENCIES_COMMON)
  
DEPENDENCIES_LV2_UI = \
  $(DEPENDENCIES_COMMON) \
  $(DEPENDENCIES_COMMON_UI)
  
DEPENDENCIES_LV2TTL_GEN = \
  LIBPTHREAD \
  LIBDL \
  LSP_COMMON_LIB

ifeq ($(PLATFORM),Linux)
  DEPENDENCIES_LV2 += \
    LIBPTHREAD \
    LIBDL \
    LIBLV2 \
    LIBSNDFILE \
    LIBCAIRO
    
  DEPENDENCIES_LV2_UI += \
    LIBPTHREAD \
    LIBDL \
    LIBLV2 \
    LIBSNDFILE \
    LIBX11 \
    LIBXRANDR \
    LIBCAIRO \
    LIBFREETYPE
endif

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES_LV2 += \
    LIBPTHREAD \
    LIBDL \
    LIBLV2 \
    LIBSNDFILE \
    LIBCAIRO
    
  DEPENDENCIES_LV2_UI += \
    LIBPTHREAD \
    LIBDL \
    LIBLV2 \
    LIBSNDFILE \
    LIBX11 \
    LIBXRANDR \
    LIBCAIRO \
    LIBFREETYPE
endif

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES_LV2 += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM \
    LIBMPR \
    LIBGDI32 \
    LIBD2D1 \
    LIBOLE \
    LIBWINCODEC \
    LIBDWRITE \
    LIBUUID
    
  DEPENDENCIES_LV2_UI += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM \
    LIBMPR \
    LIBGDI32 \
    LIBD2D1 \
    LIBOLE \
    LIBWINCODEC \
    LIBDWRITE \
    LIBUUID
endif

#------------------------------------------------------------------------------
# VST build dependencies
DEPENDENCIES_VST2 = \
  $(DEPENDENCIES_COMMON) \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_VST2_WRAP = \
  LIBDL \
  LSP_COMMON_LIB \
  LSP_3RD_PARTY

ifeq ($(PLATFORM),Linux)
  DEPENDENCIES_VST2 += \
    LIBPTHREAD \
    LIBDL \
    LIBSNDFILE \
    LIBX11 \
    LIBXRANDR \
    LIBCAIRO \
    LIBFREETYPE
endif

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES_VST2 += \
    LIBPTHREAD \
    LIBDL \
    LIBSNDFILE \
    LIBX11 \
    LIBXRANDR \
    LIBCAIRO \
    LIBFREETYPE
endif

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES_VST2 += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM \
    LIBMPR \
    LIBGDI32 \
    LIBD2D1 \
    LIBOLE \
    LIBWINCODEC \
    LIBDWRITE \
    LIBUUID
endif

#------------------------------------------------------------------------------
# VST build dependencies
DEPENDENCIES_CLAP = \
  $(DEPENDENCIES_COMMON) \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_CLAP_WRAP = \
  LIBDL \
  LSP_COMMON_LIB \
  LSP_3RD_PARTY

ifeq ($(PLATFORM),Linux)
  DEPENDENCIES_CLAP += \
    LIBPTHREAD \
    LIBDL \
    LIBSNDFILE \
    LIBX11 \
    LIBXRANDR \
    LIBCAIRO \
    LIBFREETYPE
endif

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES_CLAP += \
    LIBPTHREAD \
    LIBDL \
    LIBSNDFILE \
    LIBX11 \
    LIBXRANDR \
    LIBCAIRO \
    LIBFREETYPE
endif

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES_CLAP += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM \
    LIBMPR \
    LIBGDI32 \
    LIBD2D1 \
    LIBOLE \
    LIBWINCODEC \
    LIBDWRITE \
    LIBUUID
endif

#------------------------------------------------------------------------------
# List of dependencies
DEPENDENCIES = \
  $(DEPENDENCIES_PLUGINS) \
  $(DEPENDENCIES_JACK) \
  $(DEPENDENCIES_JACK_UI) \
  $(DEPENDENCIES_JACK_WRAP) \
  $(DEPENDENCIES_LADSPA) \
  $(DEPENDENCIES_LV2) \
  $(DEPENDENCIES_LV2_UI) \
  $(DEPENDENCIES_LV2TTL_GEN) \
  $(DEPENDENCIES_VST2) \
  $(DEPENDENCIES_CLAP)

#------------------------------------------------------------------------------
# All possible dependencies
ALL_DEPENDENCIES = \
  $(DEPENDENCIES_COMMON) \
  $(DEPENDENCIES_COMMON_UI) \
  $(TEST_DEPENDENCIES) \
  $(TEST_DEPENDENCIES_UI) \
  LIBJACK \
  LIBGL \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBDL \
  LIBICONV \
  LIBFREETYPE \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBD2D1 \
  LIBOLE \
  LIBWINCODEC
