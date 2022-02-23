/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 22 мая 2021 г.
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

#include <private/ctl/PluginWindowTemplate.h>

namespace lsp
{
    namespace ctl
    {

        //-----------------------------------------------------------------
        // Plugin window
        const ctl_class_t PluginWindowTemplate::metadata = { "PluginWindowTemplate", &Widget::metadata };

        PluginWindowTemplate::PluginWindowTemplate(ui::IWrapper *src, PluginWindow *window): Widget(src, window->widget())
        {
            pClass          = &metadata;
            pWindow         = window;
        }

        PluginWindowTemplate::~PluginWindowTemplate()
        {
        }

        void PluginWindowTemplate::begin(ui::UIContext *ctx)
        {
            Widget::begin(ctx);
        }

        void PluginWindowTemplate::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            Widget::set(ctx, name, value);
        }

        status_t PluginWindowTemplate::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            return (wnd != NULL) ? wnd->add(child->widget()) : STATUS_BAD_STATE;
        }

        void PluginWindowTemplate::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
        }
    }
}


