#
# Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
DEFAULT_FEATURES = clap doc ladspa lv2 ui vst2 vst3 xdg

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

DEPENDENCIES_COMMON_UI = \
  LSP_R3D_BASE_LIB \
  LSP_R3D_IFACE \
  LSP_TK_LIB \
  LSP_WS_LIB

DEPENDENCIES_BIN =

DEPENDENCIES_UI_BIN =

# Testing
TEST_DEPENDENCIES = \
  LSP_TEST_FW

TEST_DEPENDENCIES_UI = \
  LSP_R3D_COMMON_LIB \
  LSP_TEST_FW

# CLAP dependencies
DEPENDENCIES_CLAP = \
  $(DEPENDENCIES_COMMON)

DEPENDENCIES_CLAP_UI = \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_CLAP_BIN =

DEPENDENCIES_CLAP_UI_BIN =

# GStreamer dependencies
DEPENDENCIES_GST = \
  $(DEPENDENCIES_COMMON)

DEPENDENCIES_GST_WRAP = \
  LSP_COMMON_LIB

DEPENDENCIES_GST_BIN = 

# JACK dependencies
DEPENDENCIES_JACK = \
  $(DEPENDENCIES_COMMON)

DEPENDENCIES_JACK_UI = \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_JACK_WRAP = \
  LSP_COMMON_LIB

DEPENDENCIES_JACK_BIN =

DEPENDENCIES_JACK_UI_BIN =

# LADSPA dependencies
DEPENDENCIES_LADSPA = \
  $(DEPENDENCIES_COMMON)

DEPENDENCIES_LADSPA_BIN =

# LV2 dependencies
DEPENDENCIES_LV2 = \
  $(DEPENDENCIES_COMMON)

DEPENDENCIES_LV2_UI = \
  $(DEPENDENCIES_COMMON) \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_LV2TTL_GEN = \
  LIBPTHREAD \
  LIBDL \
  LSP_COMMON_LIB
  
DEPENDENCIES_LV2_BIN =

DEPENDENCIES_LV2_UI_BIN =

# VST2 dependencies
DEPENDENCIES_VST2 = \
  $(DEPENDENCIES_COMMON)
  
DEPENDENCIES_VST2_UI = \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_VST2_WRAP = \
  LSP_COMMON_LIB \
  LSP_3RD_PARTY

DEPENDENCIES_VST2_BIN =

DEPENDENCIES_VST2_UI_BIN =

# VST3 dependencies
DEPENDENCIES_VST3 = \
  $(DEPENDENCIES_COMMON)
  
DEPENDENCIES_VST3_UI = \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_VST3_BIN =

DEPENDENCIES_VST3_UI_BIN =

#------------------------------------------------------------------------------
# Linux-specific dependencies
LINUX_DEPENDENCIES_COMMON =

LINUX_DEPENDENCIES_COMMON_UI =

LINUX_DEPENDENCIES_BIN =

LINUX_DEPENDENCIES_UI_BIN =

LINUX_TEST_DEPENDENCIES = 

LINUX_TEST_DEPENDENCIES_UI = \
  LSP_R3D_GLX_LIB \
  LIBGL

# CLAP dependencies
LINUX_DEPENDENCIES_CLAP = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE

LINUX_DEPENDENCIES_CLAP_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

LINUX_DEPENDENCIES_CLAP_BIN =

LINUX_DEPENDENCIES_CLAP_UI_BIN = \
  LSP_R3D_GLX_LIB

# GStreamer dependencies 
LINUX_DEPENDENCIES_GST = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBGSTREAMER_AUDIO

LINUX_DEPENDENCIES_GST_WRAP = \
  LIBDL \
  LIBGSTREAMER_AUDIO
  
LINUX_DEPENDENCIES_GST_BIN =

# Jack dependencies
LINUX_DEPENDENCIES_JACK = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBJACK \
  LIBSNDFILE

LINUX_DEPENDENCIES_JACK_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBJACK \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

LINUX_DEPENDENCIES_JACK_WRAP = \
  LIBDL

LINUX_DEPENDENCIES_JACK_BIN =
  
LINUX_DEPENDENCIES_JACK_UI_BIN = \
  LSP_R3D_GLX_LIB

# LADSPA dependencies
LINUX_DEPENDENCIES_LADSPA = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE
  
LINUX_DEPENDENCIES_LADSPA_BIN =

# LV2 dependencies
LINUX_DEPENDENCIES_LV2 = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBCAIRO
    
LINUX_DEPENDENCIES_LV2_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

LINUX_DEPENDENCIES_LV2_BIN =

LINUX_DEPENDENCIES_LV2_UI_BIN = \
  LSP_R3D_GLX_LIB

# VST2 dependencies
LINUX_DEPENDENCIES_VST2 = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE
  
LINUX_DEPENDENCIES_VST2_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

LINUX_DEPENDENCIES_VST2_WRAP = \
  LIBDL

LINUX_DEPENDENCIES_VST2_BIN =

LINUX_DEPENDENCIES_VST2_UI_BIN = \
  LSP_R3D_GLX_LIB

# VST3 dependencies
LINUX_DEPENDENCIES_VST3 = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE

LINUX_DEPENDENCIES_VST3_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

LINUX_DEPENDENCIES_VST3_BIN =

LINUX_DEPENDENCIES_VST3_UI_BIN = \
  LSP_R3D_GLX_LIB

ifeq ($(PLATFORM),Linux)
  DEFAULT_FEATURES             += jack
  
  DEPENDENCIES_COMMON          += $(LINUX_DEPENDENCIES_COMMON)
  DEPENDENCIES_COMMON_UI       += $(LINUX_DEPENDENCIES_COMMON_UI)
  DEPENDENCIES_BIN             += $(LINUX_DEPENDENCIES_BIN)
  DEPENDENCIES_UI_BIN          += $(LINUX_DEPENDENCIES_UI_BIN)
  TEST_DEPENDENCIES            += $(LINUX_TEST_DEPENDENCIES)
  TEST_DEPENDENCIES_UI         += $(LINUX_TEST_DEPENDENCIES_UI)

  DEPENDENCIES_CLAP            += $(LINUX_DEPENDENCIES_CLAP)
  DEPENDENCIES_CLAP_UI         += $(LINUX_DEPENDENCIES_CLAP_UI)
  DEPENDENCIES_CLAP_BIN        += $(LINUX_DEPENDENCIES_CLAP_BIN)
  DEPENDENCIES_CLAP_UI_BIN     += $(LINUX_DEPENDENCIES_CLAP_UI_BIN)
  
  DEPENDENCIES_GST             += $(LINUX_DEPENDENCIES_GST)
  DEPENDENCIES_GST_WRAP        += $(LINUX_DEPENDENCIES_GST_WRAP)
  DEPENDENCIES_GST_BIN         += $(LINUX_DEPENDENCIES_GST_BIN)

  DEPENDENCIES_JACK            += $(LINUX_DEPENDENCIES_JACK)
  DEPENDENCIES_JACK_UI         += $(LINUX_DEPENDENCIES_JACK_UI)
  DEPENDENCIES_JACK_WRAP       += $(LINUX_DEPENDENCIES_JACK_WRAP)
  DEPENDENCIES_JACK_BIN        += $(LINUX_DEPENDENCIES_JACK_BIN)
  DEPENDENCIES_JACK_UI_BIN     += $(LINUX_DEPENDENCIES_JACK_UI_BIN)
  
  DEPENDENCIES_LADSPA          += $(LINUX_DEPENDENCIES_LADSPA)
  DEPENDENCIES_LADSPA_BIN      += $(LINUX_DEPENDENCIES_LADSPA_BIN)
  
  DEPENDENCIES_LV2             += $(LINUX_DEPENDENCIES_LV2)
  DEPENDENCIES_LV2_UI          += $(LINUX_DEPENDENCIES_LV2_UI)
  DEPENDENCIES_LV2_BIN         += $(LINUX_DEPENDENCIES_LV2_BIN)
  DEPENDENCIES_LV2_UI_BIN      += $(LINUX_DEPENDENCIES_LV2_UI_BIN)
  
  DEPENDENCIES_VST2            += $(LINUX_DEPENDENCIES_VST2)
  DEPENDENCIES_VST2_UI         += $(LINUX_DEPENDENCIES_VST2_UI)
  DEPENDENCIES_VST2_WRAP       += $(LINUX_DEPENDENCIES_VST2_WRAP)
  DEPENDENCIES_VST2_BIN        += $(LINUX_DEPENDENCIES_VST2_BIN)
  DEPENDENCIES_VST2_UI_BIN     += $(LINUX_DEPENDENCIES_VST2_UI_BIN)
  
  DEPENDENCIES_VST3            += $(LINUX_DEPENDENCIES_VST3)
  DEPENDENCIES_VST3_UI         += $(LINUX_DEPENDENCIES_VST3_UI)
  DEPENDENCIES_VST3_BIN        += $(LINUX_DEPENDENCIES_VST3_BIN)
  DEPENDENCIES_VST3_UI_BIN     += $(LINUX_DEPENDENCIES_VST3_UI_BIN)
endif

#------------------------------------------------------------------------------
# BSD-specific dependencies
BSD_DEPENDENCIES_COMMON = \
  LIBICONV

BSD_DEPENDENCIES_COMMON_UI =

BSD_DEPENDENCIES_BIN =

BSD_DEPENDENCIES_UI_BIN =

BSD_TEST_DEPENDENCIES = 

BSD_TEST_DEPENDENCIES_UI = \
  LSP_R3D_GLX_LIB \
  LIBGL

# CLAP dependencies
BSD_DEPENDENCIES_CLAP = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE

BSD_DEPENDENCIES_CLAP_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

BSD_DEPENDENCIES_CLAP_BIN =

BSD_DEPENDENCIES_CLAP_UI_BIN = \
  LSP_R3D_GLX_LIB

# GStreamer dependencies 
BSD_DEPENDENCIES_GST = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBGSTREAMER_AUDIO

BSD_DEPENDENCIES_GST_WRAP = \
  LIBDL \
  LIBGSTREAMER_AUDIO

BSD_DEPENDENCIES_GST_BIN =

# Jack dependencies
BSD_DEPENDENCIES_JACK = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBJACK \
  LIBSNDFILE

BSD_DEPENDENCIES_JACK_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBJACK \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

BSD_DEPENDENCIES_JACK_WRAP = \
  LIBDL

BSD_DEPENDENCIES_JACK_BIN = 

BSD_DEPENDENCIES_JACK_UI_BIN = \
  LSP_R3D_GLX_LIB

# LADSPA dependencies
BSD_DEPENDENCIES_LADSPA = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE

BSD_DEPENDENCIES_LADSPA_BIN =

# LV2 dependencies
BSD_DEPENDENCIES_LV2 = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBCAIRO
    
BSD_DEPENDENCIES_LV2_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

BSD_DEPENDENCIES_LV2_BIN =

BSD_DEPENDENCIES_LV2_UI_BIN = \
  LSP_R3D_GLX_LIB

# VST2 dependencies
BSD_DEPENDENCIES_VST2 = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE

BSD_DEPENDENCIES_VST2_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

BSD_DEPENDENCIES_VST2_WRAP = \
  LIBDL

BSD_DEPENDENCIES_VST2_BIN = \

BSD_DEPENDENCIES_VST2_UI_BIN = \
  LSP_R3D_GLX_LIB

# VST3 dependencies
BSD_DEPENDENCIES_VST3 = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE
  
BSD_DEPENDENCIES_VST3_UI = \
  LIBPTHREAD \
  LIBDL \
  LIBRT \
  LIBSNDFILE \
  LIBX11 \
  LIBXRANDR \
  LIBCAIRO \
  LIBFREETYPE

BSD_DEPENDENCIES_VST3_BIN =

BSD_DEPENDENCIES_VST3_UI_BIN = \
  LSP_R3D_GLX_LIB

ifeq ($(PLATFORM),BSD)
  DEFAULT_FEATURES             += jack
  
  DEPENDENCIES_COMMON          += $(BSD_DEPENDENCIES_COMMON)
  DEPENDENCIES_COMMON_UI       += $(BSD_DEPENDENCIES_COMMON_UI)
  DEPENDENCIES_BIN             += $(BSD_DEPENDENCIES_BIN)
  DEPENDENCIES_UI_BIN          += $(BSD_DEPENDENCIES_UI_BIN)
  TEST_DEPENDENCIES            += $(BSD_TEST_DEPENDENCIES)
  TEST_DEPENDENCIES_UI         += $(BSD_TEST_DEPENDENCIES_UI)

  DEPENDENCIES_CLAP            += $(BSD_DEPENDENCIES_CLAP)
  DEPENDENCIES_CLAP_UI         += $(BSD_DEPENDENCIES_CLAP_UI)
  DEPENDENCIES_CLAP_BIN        += $(BSD_DEPENDENCIES_CLAP_BIN)
  DEPENDENCIES_CLAP_UI_BIN     += $(BSD_DEPENDENCIES_CLAP_UI_BIN)
  
  DEPENDENCIES_GST             += $(BSD_DEPENDENCIES_GST)
  DEPENDENCIES_GST_WRAP        += $(BSD_DEPENDENCIES_GST_WRAP)
  DEPENDENCIES_GST_BIN         += $(BSD_DEPENDENCIES_GST_BIN)

  DEPENDENCIES_JACK            += $(BSD_DEPENDENCIES_JACK)
  DEPENDENCIES_JACK_UI         += $(BSD_DEPENDENCIES_JACK_UI)
  DEPENDENCIES_JACK_WRAP       += $(BSD_DEPENDENCIES_JACK_WRAP)
  DEPENDENCIES_JACK_BIN        += $(BSD_DEPENDENCIES_JACK_BIN)
  DEPENDENCIES_JACK_UI_BIN     += $(BSD_DEPENDENCIES_JACK_UI_BIN)
  
  DEPENDENCIES_LADSPA          += $(BSD_DEPENDENCIES_LADSPA)
  DEPENDENCIES_LADSPA_BIN      += $(BSD_DEPENDENCIES_LADSPA_BIN)
  
  DEPENDENCIES_LV2             += $(BSD_DEPENDENCIES_LV2)
  DEPENDENCIES_LV2_UI          += $(BSD_DEPENDENCIES_LV2_UI)
  DEPENDENCIES_LV2_BIN         += $(BSD_DEPENDENCIES_LV2_BIN)
  DEPENDENCIES_LV2_UI_BIN      += $(BSD_DEPENDENCIES_LV2_UI_BIN)
  
  DEPENDENCIES_VST2            += $(BSD_DEPENDENCIES_VST2)
  DEPENDENCIES_VST2_UI         += $(BSD_DEPENDENCIES_VST2_UI)
  DEPENDENCIES_VST2_WRAP       += $(BSD_DEPENDENCIES_VST2_WRAP)
  DEPENDENCIES_VST2_BIN        += $(BSD_DEPENDENCIES_VST2_BIN)
  DEPENDENCIES_VST2_UI_BIN     += $(BSD_DEPENDENCIES_VST2_UI_BIN)
  
  DEPENDENCIES_VST3            += $(BSD_DEPENDENCIES_VST3)
  DEPENDENCIES_VST3_UI         += $(BSD_DEPENDENCIES_VST3_UI)
  DEPENDENCIES_VST3_BIN        += $(BSD_DEPENDENCIES_VST3_BIN)
  DEPENDENCIES_VST3_UI_BIN     += $(BSD_DEPENDENCIES_VST3_UI_BIN)
endif

#------------------------------------------------------------------------------
# Windows-specific dependencies
WINDOWS_DEPENDENCIES_COMMON = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBMPR

WINDOWS_DEPENDENCIES_COMMON_UI = \
  LIBGDI32 \
  LIBD2D1 \
  LIBOLE \
  LIBWINCODEC \
  LIBDWRITE \
  LIBUUID

WINDOWS_DEPENDENCIES_BIN =

WINDOWS_DEPENDENCIES_UI_BIN =

WINDOWS_TEST_DEPENDENCIES = 

WINDOWS_TEST_DEPENDENCIES_UI = \
  LSP_R3D_WGL_LIB \
  LIBOPENGL32

# CLAP dependencies
WINDOWS_DEPENDENCIES_CLAP = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBMPR \
  LIBOLE \
  LIBWINCODEC \
  LIBUUID

WINDOWS_DEPENDENCIES_CLAP_UI = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBMPR \
  LIBOLE \
  LIBWINCODEC \
  LIBUUID \
  LIBGDI32 \
  LIBD2D1 \
  LIBDWRITE

WINDOWS_DEPENDENCIES_CLAP_BIN =

WINDOWS_DEPENDENCIES_CLAP_UI_BIN = \
  LSP_R3D_WGL_LIB

# GStreamer dependencies 
WINDOWS_DEPENDENCIES_GST = 

WINDOWS_DEPENDENCIES_GST_WRAP =

WINDOWS_DEPENDENCIES_GST_BIN =

# Jack dependencies
WINDOWS_DEPENDENCIES_JACK = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBMPR \
  LIBJACK

WINDOWS_DEPENDENCIES_JACK_UI = \
  LIBGDI32 \
  LIBD2D1 \
  LIBOLE \
  LIBWINCODEC \
  LIBDWRITE \
  LIBUUID

WINDOWS_DEPENDENCIES_JACK_WRAP =

WINDOWS_DEPENDENCIES_JACK_BIN =

WINDOWS_DEPENDENCIES_JACK_UI_BIN = \
  LSP_R3D_WGL_LIB

# LADSPA dependencies
WINDOWS_DEPENDENCIES_LADSPA = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBMPR

WINDOWS_DEPENDENCIES_LADSPA_BIN =

# LV2 dependencies
WINDOWS_DEPENDENCIES_LV2 = \
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
    
WINDOWS_DEPENDENCIES_LV2_UI = \
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

WINDOWS_DEPENDENCIES_LV2_BIN = \

WINDOWS_DEPENDENCIES_LV2_UI_BIN = \
  LSP_R3D_WGL_LIB

# VST2 dependencies
WINDOWS_DEPENDENCIES_VST2 = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBMPR \
  LIBOLE \
  LIBWINCODEC \
  LIBUUID
  
WINDOWS_DEPENDENCIES_VST2_UI = \
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

WINDOWS_DEPENDENCIES_VST2_WRAP = \
  LIBADVAPI

WINDOWS_DEPENDENCIES_VST2_BIN =

WINDOWS_DEPENDENCIES_VST2_UI_BIN = \
  LSP_R3D_WGL_LIB

# VST3 dependencies
WINDOWS_DEPENDENCIES_VST3 = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM \
  LIBMPR \
  LIBOLE \
  LIBWINCODEC \
  LIBUUID
  
WINDOWS_DEPENDENCIES_VST3_UI = \
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

WINDOWS_DEPENDENCIES_VST3_BIN =

WINDOWS_DEPENDENCIES_VST3_UIBIN = \
  LSP_R3D_WGL_LIB

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES_COMMON          += $(WINDOWS_DEPENDENCIES_COMMON)
  DEPENDENCIES_COMMON_UI       += $(WINDOWS_DEPENDENCIES_COMMON_UI)
  DEPENDENCIES_BIN             += $(WINDOWS_DEPENDENCIES_BIN)
  DEPENDENCIES_UI_BIN          += $(WINDOWS_DEPENDENCIES_UI_BIN)
  TEST_DEPENDENCIES            += $(WINDOWS_TEST_DEPENDENCIES)
  TEST_DEPENDENCIES_UI         += $(WINDOWS_TEST_DEPENDENCIES_UI)

  DEPENDENCIES_CLAP            += $(WINDOWS_DEPENDENCIES_CLAP)
  DEPENDENCIES_CLAP_UI         += $(WINDOWS_DEPENDENCIES_CLAP_UI)
  DEPENDENCIES_CLAP_BIN        += $(WINDOWS_DEPENDENCIES_CLAP_BIN)
  DEPENDENCIES_CLAP_UI_BIN     += $(WINDOWS_DEPENDENCIES_CLAP_UI_BIN)
  
  DEPENDENCIES_GST             += $(WINDOWS_DEPENDENCIES_GST)
  DEPENDENCIES_GST_WRAP        += $(WINDOWS_DEPENDENCIES_GST_WRAP)
  DEPENDENCIES_GST_BIN         += $(WINDOWS_DEPENDENCIES_GST_BIN)

  DEPENDENCIES_JACK            += $(WINDOWS_DEPENDENCIES_JACK)
  DEPENDENCIES_JACK_UI         += $(WINDOWS_DEPENDENCIES_JACK_UI)
  DEPENDENCIES_JACK_WRAP       += $(WINDOWS_DEPENDENCIES_JACK_WRAP)
  DEPENDENCIES_JACK_BIN        += $(WINDOWS_DEPENDENCIES_JACK_BIN)
  DEPENDENCIES_JACK_UI_BIN     += $(WINDOWS_DEPENDENCIES_JACK_UI_BIN)
  
  DEPENDENCIES_LADSPA          += $(WINDOWS_DEPENDENCIES_LADSPA)
  DEPENDENCIES_LADSPA_BIN      += $(WINDOWS_DEPENDENCIES_LADSPA_BIN)
  
  DEPENDENCIES_LV2             += $(WINDOWS_DEPENDENCIES_LV2)
  DEPENDENCIES_LV2_UI          += $(WINDOWS_DEPENDENCIES_LV2_UI)
  DEPENDENCIES_LV2_BIN         += $(WINDOWS_DEPENDENCIES_LV2_BIN)
  DEPENDENCIES_LV2_UI_BIN      += $(WINDOWS_DEPENDENCIES_LV2_UI_BIN)
  
  DEPENDENCIES_VST2            += $(WINDOWS_DEPENDENCIES_VST2)
  DEPENDENCIES_VST2_UI         += $(WINDOWS_DEPENDENCIES_VST2_UI)
  DEPENDENCIES_VST2_WRAP       += $(WINDOWS_DEPENDENCIES_VST2_WRAP)
  DEPENDENCIES_VST2_BIN        += $(WINDOWS_DEPENDENCIES_VST2_BIN)
  DEPENDENCIES_VST2_UI_BIN     += $(WINDOWS_DEPENDENCIES_VST2_UI_BIN)
  
  DEPENDENCIES_VST3            += $(WINDOWS_DEPENDENCIES_VST3)
  DEPENDENCIES_VST3_UI         += $(WINDOWS_DEPENDENCIES_VST3_UI)
  DEPENDENCIES_VST3_BIN        += $(WINDOWS_DEPENDENCIES_VST3_BIN)
  DEPENDENCIES_VST3_UI_BIN     += $(WINDOWS_DEPENDENCIES_VST3_UI_BIN)
endif

#------------------------------------------------------------------------------
# List of dependencies
DEPENDENCIES = \
  $(DEPENDENCIES_PLUGINS) \
  $(DEPENDENCIES_CLAP) \
  $(DEPENDENCIES_CLAP_UI) \
  $(DEPENDENCIES_GST) \
  $(DEPENDENCIES_GST_WRAP) \
  $(DEPENDENCIES_JACK) \
  $(DEPENDENCIES_JACK_UI) \
  $(DEPENDENCIES_JACK_WRAP) \
  $(DEPENDENCIES_LADSPA) \
  $(DEPENDENCIES_LV2) \
  $(DEPENDENCIES_LV2_UI) \
  $(DEPENDENCIES_LV2TTL_GEN) \
  $(DEPENDENCIES_VST2) \
  $(DEPENDENCIES_VST2_UI) \
  $(DEPENDENCIES_VST3) \
  $(DEPENDENCIES_VST3_UI)

#------------------------------------------------------------------------------
# All possible dependencies
ALL_DEPENDENCIES = \
  $(DEPENDENCIES_PLUGINS) \
  \
  $(DEPENDENCIES_COMMON) \
  $(DEPENDENCIES_COMMON_UI) \
  $(DEPENDENCIES_BIN) \
  $(DEPENDENCIES_UI_BIN) \
  $(TEST_DEPENDENCIES) \
  $(TEST_DEPENDENCIES_UI) \
  $(DEPENDENCIES_CLAP) \
  $(DEPENDENCIES_CLAP_UI) \
  $(DEPENDENCIES_CLAP_BIN) \
  $(DEPENDENCIES_CLAP_UI_BIN) \
  $(DEPENDENCIES_GST) \
  $(DEPENDENCIES_GST_BIN) \
  $(DEPENDENCIES_GST_WRAP) \
  $(DEPENDENCIES_JACK) \
  $(DEPENDENCIES_JACK_UI) \
  $(DEPENDENCIES_JACK_WRAP) \
  $(DEPENDENCIES_JACK_BIN) \
  $(DEPENDENCIES_JACK_UI_BIN) \
  $(DEPENDENCIES_LADSPA) \
  $(DEPENDENCIES_LADSPA_BIN) \
  $(DEPENDENCIES_LV2) \
  $(DEPENDENCIES_LV2_UI) \
  $(DEPENDENCIES_LV2TTL_GEN) \
  $(DEPENDENCIES_LV2_BIN) \
  $(DEPENDENCIES_LV2_UI_BIN) \
  $(DEPENDENCIES_VST2) \
  $(DEPENDENCIES_VST2_UI) \
  $(DEPENDENCIES_VST2_BIN) \
  $(DEPENDENCIES_VST2_UI_BIN) \
  $(DEPENDENCIES_VST3) \
  $(DEPENDENCIES_VST3_UI) \
  $(DEPENDENCIES_VST3_BIN) \
  $(DEPENDENCIES_VST3_UI_BIN) \
  \
  $(LINUX_DEPENDENCIES_COMMON) \
  $(LINUX_DEPENDENCIES_COMMON_UI) \
  $(LINUX_DEPENDENCIES_BIN) \
  $(LINUX_TEST_DEPENDENCIES) \
  $(LINUX_TEST_DEPENDENCIES_UI) \
  $(LINUX_DEPENDENCIES_CLAP) \
  $(LINUX_DEPENDENCIES_CLAP_UI) \
  $(LINUX_DEPENDENCIES_CLAP_BIN) \
  $(LINUX_DEPENDENCIES_CLAP_UI_BIN) \
  $(LINUX_DEPENDENCIES_GST) \
  $(LINUX_DEPENDENCIES_GST_BIN) \
  $(LINUX_DEPENDENCIES_GST_WRAP) \
  $(LINUX_DEPENDENCIES_JACK) \
  $(LINUX_DEPENDENCIES_JACK_UI) \
  $(LINUX_DEPENDENCIES_JACK_WRAP) \
  $(LINUX_DEPENDENCIES_JACK_BIN) \
  $(LINUX_DEPENDENCIES_JACK_UI_BIN) \
  $(LINUX_DEPENDENCIES_LADSPA) \
  $(LINUX_DEPENDENCIES_LADSPA_BIN) \
  $(LINUX_DEPENDENCIES_LV2) \
  $(LINUX_DEPENDENCIES_LV2_UI) \
  $(LINUX_DEPENDENCIES_LV2TTL_GEN) \
  $(LINUX_DEPENDENCIES_LV2_BIN) \
  $(LINUX_DEPENDENCIES_LV2_UI_BIN) \
  $(LINUX_DEPENDENCIES_VST2) \
  $(LINUX_DEPENDENCIES_VST2_UI) \
  $(LINUX_DEPENDENCIES_VST2_BIN) \
  $(LINUX_DEPENDENCIES_VST2_UI_BIN) \
  $(LINUX_DEPENDENCIES_VST3) \
  $(LINUX_DEPENDENCIES_VST3_UI) \
  $(LINUX_DEPENDENCIES_VST3_BIN) \
  $(LINUX_DEPENDENCIES_VST3_UI_BIN) \
  \
  $(BSD_DEPENDENCIES_COMMON) \
  $(BSD_DEPENDENCIES_COMMON_UI) \
  $(BSD_DEPENDENCIES_BIN) \
  $(BSD_TEST_DEPENDENCIES) \
  $(BSD_TEST_DEPENDENCIES_UI) \
  $(BSD_DEPENDENCIES_CLAP) \
  $(BSD_DEPENDENCIES_CLAP_UI) \
  $(BSD_DEPENDENCIES_CLAP_BIN) \
  $(BSD_DEPENDENCIES_CLAP_UI_BIN) \
  $(BSD_DEPENDENCIES_GST) \
  $(BSD_DEPENDENCIES_GST_WRAP) \
  $(BSD_DEPENDENCIES_GST_BIN) \
  $(BSD_DEPENDENCIES_JACK) \
  $(BSD_DEPENDENCIES_JACK_UI) \
  $(BSD_DEPENDENCIES_JACK_WRAP) \
  $(BSD_DEPENDENCIES_JACK_BIN) \
  $(BSD_DEPENDENCIES_JACK_UI_BIN) \
  $(BSD_DEPENDENCIES_LADSPA) \
  $(BSD_DEPENDENCIES_LADSPA_BIN) \
  $(BSD_DEPENDENCIES_LV2) \
  $(BSD_DEPENDENCIES_LV2_UI) \
  $(BSD_DEPENDENCIES_LV2TTL_GEN) \
  $(BSD_DEPENDENCIES_LV2_BIN) \
  $(BSD_DEPENDENCIES_LV2_UI_BIN) \
  $(BSD_DEPENDENCIES_VST2) \
  $(BSD_DEPENDENCIES_VST2_UI) \
  $(BSD_DEPENDENCIES_VST2_BIN) \
  $(BSD_DEPENDENCIES_VST2_UI_BIN) \
  $(BSD_DEPENDENCIES_VST3) \
  $(BSD_DEPENDENCIES_VST3_UI) \
  $(BSD_DEPENDENCIES_VST3_BIN) \
  $(BSD_DEPENDENCIES_VST3_UI_BIN) \
  \
  $(WINDOWS_DEPENDENCIES_COMMON) \
  $(WINDOWS_DEPENDENCIES_COMMON_UI) \
  $(WINDOWS_DEPENDENCIES_BIN) \
  $(WINDOWS_TEST_DEPENDENCIES) \
  $(WINDOWS_TEST_DEPENDENCIES_UI) \
  $(WINDOWS_DEPENDENCIES_CLAP) \
  $(WINDOWS_DEPENDENCIES_CLAP_UI) \
  $(WINDOWS_DEPENDENCIES_CLAP_BIN) \
  $(WINDOWS_DEPENDENCIES_CLAP_UI_BIN) \
  $(WINDOWS_DEPENDENCIES_GST) \
  $(WINDOWS_DEPENDENCIES_GST_WRAP) \
  $(WINDOWS_DEPENDENCIES_GST_BIN) \
  $(WINDOWS_DEPENDENCIES_JACK) \
  $(WINDOWS_DEPENDENCIES_JACK_UI) \
  $(WINDOWS_DEPENDENCIES_JACK_WRAP) \
  $(WINDOWS_DEPENDENCIES_JACK_BIN) \
  $(WINDOWS_DEPENDENCIES_JACK_UI_BIN) \
  $(WINDOWS_DEPENDENCIES_LADSPA) \
  $(WINDOWS_DEPENDENCIES_LADSPA_BIN) \
  $(WINDOWS_DEPENDENCIES_LV2) \
  $(WINDOWS_DEPENDENCIES_LV2_UI) \
  $(WINDOWS_DEPENDENCIES_LV2TTL_GEN) \
  $(WINDOWS_DEPENDENCIES_LV2_BIN) \
  $(WINDOWS_DEPENDENCIES_LV2_UI_BIN) \
  $(WINDOWS_DEPENDENCIES_VST2) \
  $(WINDOWS_DEPENDENCIES_VST2_UI) \
  $(WINDOWS_DEPENDENCIES_VST2_BIN) \
  $(WINDOWS_DEPENDENCIES_VST2_UI_BIN) \
  $(WINDOWS_DEPENDENCIES_VST3) \
  $(WINDOWS_DEPENDENCIES_VST3_UI) \
  $(WINDOWS_DEPENDENCIES_VST3_BIN) \
  $(WINDOWS_DEPENDENCIES_VST3_UI_BIN)

