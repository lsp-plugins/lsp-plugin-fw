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

# Package version
ARTIFACT_ID                 = LSP_PLUGIN_FW
ARTIFACT_NAME               = lsp-plugin-fw
ARTIFACT_DESC               = Plugin framework for building LSP Plugins
ARTIFACT_HEADERS            = lsp-plug.in
ARTIFACT_EXPORT_ALL         = 1
ARTIFACT_VERSION            = 0.5.0-devel

#------------------------------------------------------------------------------
# Plugin dependencies
DEPENDENCIES_COMMON = \
  LIBPTHREAD \
  LIBDL \
  LSP_COMMON_LIB \
  LSP_DSP_LIB \
  LSP_DSP_UNITS \
  LSP_LLTL_LIB \
  LSP_RUNTIME_LIB \
  LSP_PLUGINS_SHARED \
  LSP_3RD_PARTY

DEPENDENCIES_COMMON_UI = \
  LSP_R3D_IFACE \
  LSP_WS_LIB \
  LSP_TK_LIB

#------------------------------------------------------------------------------
# Jack build dependencies
DEPENDENCIES_JACK = \
  $(DEPENDENCIES_COMMON)

DEPENDENCIES_JACK_UI = \
  $(DEPENDENCIES_COMMON_UI)

DEPENDENCIES_JACK_WRAP = \
  LIBPTHREAD \
  LIBDL \
  LSP_COMMON_LIB

ifeq ($(PLATFORM),Linux)
  DEPENDENCIES_JACK += \
    LIBJACK \
    LIBSNDFILE

  DEPENDENCIES_JACK_UI += \
    LIBJACK \
    LIBSNDFILE \
    LIBX11 \
    LIBCAIRO
endif

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES_JACK += \
    LIBJACK \
    LIBSNDFILE
    
  DEPENDENCIES_JACK_UI += \
    LIBJACK \
    LIBSNDFILE \
    LIBX11 \
    LIBCAIRO
endif

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES             += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM
endif

#------------------------------------------------------------------------------
# List of dependencies
DEPENDENCIES = \
  $(DEPENDENCIES_PLUGINS) \
  $(DEPENDENCIES_JACK) \
  $(DEPENDENCIES_JACK_UI)

TEST_DEPENDENCIES = \
  LSP_TEST_FW \
  LSP_R3D_BASE_LIB \
  LSP_R3D_GLX_LIB

#------------------------------------------------------------------------------
# Platform-specific dependencies
ifeq ($(PLATFORM),Linux)
  TEST_DEPENDENCIES += \
    LIBGL
endif

ifeq ($(PLATFORM),BSD)
  TEST_DEPENDENCIES += \
    LIBGL
    
  DEPENDENCIES_COMMON += \
    LIBICONV
endif

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES_COMMON   += \
    LIBSHLWAPI \
    LIBWINMM \
    LIBMSACM
endif

#------------------------------------------------------------------------------
# All possible dependencies
ALL_DEPENDENCIES = \
  $(DEPENDENCIES_COMMON) \
  $(DEPENDENCIES_COMMON_UI) \
  $(TEST_DEPENDENCIES) \
  LIBJACK \
  LIBGL \
  LIBSNDFILE \
  LIBX11 \
  LIBCAIRO \
  LIBDL \
  LIBICONV \
  LIBWINNT



