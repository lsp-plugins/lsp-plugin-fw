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

# Specify description of plugin dependency
PLUGINS_COMP_DELAY_VERSION        	:= 1.0.2
PLUGINS_COMP_DELAY_NAME             := lsp-plugins-comp-delay
PLUGINS_COMP_DELAY_TYPE             := plug
PLUGINS_COMP_DELAY_URL              := https://github.com/sadko4u/$(PLUGINS_COMP_DELAY_NAME).git

# List of all plugin dependencies
PLUGIN_DEPENDENCIES = \
  PLUGINS_COMP_DELAY
