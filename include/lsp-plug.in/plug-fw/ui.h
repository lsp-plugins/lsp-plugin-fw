/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
 *
 * lsp-plugin-fw is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugin-fw is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugin-fw. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_UI_H_
#define LSP_PLUG_IN_PLUG_FW_UI_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

#define LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #include <lsp-plug.in/plug-fw/ui/const.h>

    #include <lsp-plug.in/plug-fw/ui/IPort.h>
    #include <lsp-plug.in/plug-fw/ui/IPortListener.h>
    #include <lsp-plug.in/plug-fw/ui/IWrapper.h>
    #include <lsp-plug.in/plug-fw/ui/PortResolver.h>

    #include <lsp-plug.in/plug-fw/ui/ControlPort.h>
    #include <lsp-plug.in/plug-fw/ui/PathPort.h>
    #include <lsp-plug.in/plug-fw/ui/ValuePort.h>
    #include <lsp-plug.in/plug-fw/ui/SwitchedPort.h>

    #include <lsp-plug.in/plug-fw/ui/Module.h>
    #include <lsp-plug.in/plug-fw/ui/Factory.h>
#undef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_


#endif /* LSP_PLUG_IN_PLUG_FW_UI_H_ */
