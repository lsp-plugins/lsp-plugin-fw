/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 мая 2025 г.
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

#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        const ctl_class_t DOMController::metadata = { "Controller", &Controller::metadata };

        DOMController::DOMController(ui::IWrapper *wrapper):
            Controller(wrapper)
        {
            pClass          = &metadata;
        }

        DOMController::~DOMController()
        {
        }

        void DOMController::set(ui::UIContext *ctx, const char *name, const char *value)
        {
        }

        void DOMController::begin(ui::UIContext *ctx)
        {
        }

        void DOMController::end(ui::UIContext *ctx)
        {
        }

    } /* namespace ctl */
} /* namespace lsp */


