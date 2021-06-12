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

#ifndef PRIVATE_CTL_PLUGINWINDOWTEMPLATE_H_
#define PRIVATE_CTL_PLUGINWINDOWTEMPLATE_H_

#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        class PluginWindowTemplate: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                PluginWindow       *pWindow;

            public:
                explicit PluginWindowTemplate(ui::IWrapper *src, PluginWindow *window);
                virtual ~PluginWindowTemplate();

            public:
                virtual void        begin(ui::UIContext *ctx);

                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);

                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child);

                virtual void        end(ui::UIContext *ctx);
        };
    }
}

#endif /* PRIVATE_CTL_PLUGINWINDOWTEMPLATE_H_ */
