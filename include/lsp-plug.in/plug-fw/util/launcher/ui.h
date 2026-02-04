/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 янв. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/util/launcher/plugin_metadata.h>
#include <lsp-plug.in/plug-fw/util/launcher/visual_schema.h>

#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace launcher
    {
        class UI: public ui::IWrapper, public ui::IPortListener
        {
            protected:
                meta::package_t            *pPackage;           // Package descriptor
                ui::IPort                  *pWindowWidth;
                ui::IPort                  *pWindowHeight;

            protected:
                static status_t             slot_display_idle(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_window_resize(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                        do_destroy();
                status_t                    build_ui();
                status_t                    load_package_info();
                void                        on_window_resize();

            public:
                UI(resource::ILoader * loader);
                virtual ~UI() override;
                UI(const UI &) = delete;
                UI(UI &&) = delete;

                UI & operator = (const UI &) = delete;
                UI & operator = (UI &&) = delete;

                virtual status_t            init(void *root_widget) override;
                virtual void                destroy() override;

            public: // ui::IWrapper
                virtual const meta::package_t      *package() const override;

            public: // ui::IPortListener
                virtual void notify(ui::IPort *port, size_t flags);

            public:
                status_t                    main_loop();
        };


    } /* namespace launcher */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_H_ */
