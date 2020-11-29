#
# Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
#
# This file is part of lsp-dsp-lib
#
# lsp-dsp-lib is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# lsp-dsp-lib is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with lsp-dsp-lib.  If not, see <https://www.gnu.org/licenses/>.
#

# Variables that describe dependencies
LSP_COMMON_LIB_VERSION     := 1.0.11
LSP_COMMON_LIB_NAME        := lsp-common-lib
LSP_COMMON_LIB_TYPE        := src
LSP_COMMON_LIB_URL         := https://github.com/sadko4u/$(LSP_COMMON_LIB_NAME).git

LSP_3RD_PARTY_VERSION      := 0.5.0
LSP_3RD_PARTY_NAME         := lsp-3rd-party
LSP_3RD_PARTY_TYPE         := hdr
LSP_3RD_PARTY_URL          := https://github.com/sadko4u/$(LSP_3RD_PARTY_NAME).git

LSP_DSP_LIB_VERSION        := 0.5.11
LSP_DSP_LIB_NAME           := lsp-dsp-lib
LSP_DSP_LIB_TYPE           := src
LSP_DSP_LIB_URL            := https://github.com/sadko4u/$(LSP_DSP_LIB_NAME).git

LSP_DSP_UNITS_VERSION      := 0.5.0
LSP_DSP_UNITS_NAME         := lsp-dsp-units
LSP_DSP_UNITS_TYPE         := src
LSP_DSP_UNITS_URL          := https://github.com/sadko4u/$(LSP_DSP_UNITS_NAME).git

LSP_RUNTIME_LIB_VERSION    := 0.5.6
LSP_RUNTIME_LIB_NAME       := lsp-runtime-lib
LSP_RUNTIME_LIB_TYPE       := src
LSP_RUNTIME_LIB_URL        := https://github.com/sadko4u/$(LSP_RUNTIME_LIB_NAME).git

LSP_LLTL_LIB_VERSION       := 0.5.5
LSP_LLTL_LIB_NAME          := lsp-lltl-lib
LSP_LLTL_LIB_TYPE          := src
LSP_LLTL_LIB_URL           := https://github.com/sadko4u/$(LSP_LLTL_LIB_NAME).git

LSP_WS_LIB_VERSION         := 0.5.3
LSP_WS_LIB_NAME            := lsp-ws-lib
LSP_WS_LIB_TYPE            := src
LSP_WS_LIB_URL             := https://github.com/sadko4u/$(LSP_WS_LIB_NAME).git

LSP_R3D_IFACE_VERSION      := 0.5.1
LSP_R3D_IFACE_NAME         := lsp-r3d-iface
LSP_R3D_IFACE_TYPE         := src
LSP_R3D_IFACE_URL          := https://github.com/sadko4u/$(LSP_R3D_IFACE_NAME).git

LSP_R3D_BASE_LIB_VERSION   := 0.5.4
LSP_R3D_BASE_LIB_NAME      := lsp-r3d-base-lib
LSP_R3D_BASE_LIB_TYPE      := src
LSP_R3D_BASE_LIB_URL       := https://github.com/sadko4u/$(LSP_R3D_BASE_LIB_NAME).git

LSP_R3D_GLX_LIB_VERSION    := 0.5.3
LSP_R3D_GLX_LIB_NAME       := lsp-r3d-glx-lib
LSP_R3D_GLX_LIB_TYPE       := src
LSP_R3D_GLX_LIB_URL        := https://github.com/sadko4u/$(LSP_R3D_GLX_LIB_NAME).git

LSP_WS_LIB_VERSION         := 0.5.3
LSP_WS_LIB_NAME            := lsp-ws-lib
LSP_WS_LIB_TYPE            := src
LSP_WS_LIB_URL             := https://github.com/sadko4u/$(LSP_WS_LIB_NAME).git

LSP_TK_LIB_VERSION         := 0.5.0
LSP_TK_LIB_NAME            := lsp-tk-lib
LSP_TK_LIB_TYPE            := src
LSP_TK_LIB_URL             := https://github.com/sadko4u/$(LSP_TK_LIB_NAME).git

LSP_TEST_FW_VERSION        := 1.0.6
LSP_TEST_FW_NAME           := lsp-test-fw
LSP_TEST_FW_TYPE           := src
LSP_TEST_FW_URL            := https://github.com/sadko4u/$(LSP_TEST_FW_NAME).git

LIBICONV_VERSION           := system
LIBICONV_TYPE              := opt
LIBICONV_LDFLAGS           := -liconv

LIBSNDFILE_VERSION         := system
LIBSNDFILE_NAME            := sndfile
LIBSNDFILE_TYPE            := pkg

LIBJACK_VERSION            := system
LIBJACK_NAME               := jack
LIBJACK_TYPE               := pkg

# For test (and build) purpose
LSP_PLUGINS_SHARED_VERSION              := 0.5.0
LSP_PLUGINS_SHARED_NAME                 := lsp-plugins-shared
LSP_PLUGINS_SHARED_TYPE                 := src
LSP_PLUGINS_SHARED_URL                  := https://github.com/sadko4u/$(LSP_PLUGINS_SHARED_NAME).git

LSP_PLUGINS_COMP_DELAY_VERSION          := 1.0.2
LSP_PLUGINS_COMP_DELAY_NAME             := lsp-plugins-comp-delay
LSP_PLUGINS_COMP_DELAY_TYPE             := plug
LSP_PLUGINS_COMP_DELAY_URL              := https://github.com/sadko4u/$(LSP_PLUGINS_COMP_DELAY_NAME).git

ifeq ($(PLATFORM),Windows)
  STDLIB_VERSION             := system
  STDLIB_TYPE                := opt
  STDLIB_LDFLAGS             := -lpthread -lshlwapi -lwinmm -lmsacm32
else
  STDLIB_VERSION             := system
  STDLIB_TYPE                := opt
  STDLIB_LDFLAGS             := -lpthread -ldl
endif

