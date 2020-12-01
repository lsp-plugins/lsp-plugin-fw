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
ARTIFACT_NAME               = lsp-plugin-fw
ARTIFACT_DESC               = Plugin framework for building LSP Plugins
ARTIFACT_VARS               = LSP_PLUGIN_FW
ARTIFACT_HEADERS            = lsp-plug.in
ARTIFACT_EXPORT_ALL         = 1
ARTIFACT_VERSION            = 0.5.0-devel

# List of dependencies
DEPENDENCIES = \
  STDLIB \
  LSP_COMMON_LIB \
  LSP_3RD_PARTY \
  LSP_DSP_LIB \
  LSP_DSP_UNITS \
  LSP_LLTL_LIB \
  LSP_RUNTIME_LIB \
  LSP_R3D_IFACE \
  LSP_WS_LIB \
  LSP_TK_LIB

TEST_DEPENDENCIES = \
  LIBGL \
  TEST_STDLIB \
  LSP_R3D_BASE_LIB \
  LSP_R3D_GLX_LIB \
  LSP_TEST_FW \
  LSP_PLUGINS_SHARED \
  LSP_PLUGINS_COMP_DELAY

LINUX_DEPENDENCIES = \
  LIBSNDFILE \
  LIBX11 \
  LIBCAIRO \
  LIBJACK

BSD_DEPENDENCIES = \
  LIBSNDFILE \
  LIBX11 \
  LIBCAIRO \
  LIBJACK \
  LIBICONV

# For Linux-based systems, use libsndfile
ifeq ($(PLATFORM),Linux)
  DEPENDENCIES             += $(LINUX_DEPENDENCIES)
endif

# For BSD-based systems, use libsndfile
ifeq ($(PLATFORM),BSD)
  DEPENDENCIES             += $(BSD_DEPENDENCIES)
endif

# All possible dependencies
ALL_DEPENDENCIES = \
  $(DEPENDENCIES) \
  $(TEST_DEPENDENCIES) \
  $(LINUX_DEPENDENCIES) \
  $(BSD_DEPENDENCIES)


