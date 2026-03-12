/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 февр. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_FONTSCALING_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_FONTSCALING_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * The controller for the font scaling
         */
        class FontScaling: public ui::IPortListener
        {
            protected:
                typedef struct scaling_sel_t
                {
                    ctl::FontScaling     *ctl;
                    float               scaling;
                    tk::MenuItem       *item;
                } scaling_sel_t;

            protected:
                ui::IWrapper       *pWrapper;           // Plugin wrapper
                tk::Menu           *wMenu;              // Font scaling menu

                ui::IPort          *pFontScaling;

                lltl::parray<scaling_sel_t> vFontScalingSel;

            protected: // Slots
                static status_t slot_zoom_in(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_zoom_out(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_select(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show(tk::Widget *sender, void *ptr, void *data);

            protected:
                tk::MenuItem       *create_menu_item();
                tk::Menu           *create_menu();
                status_t            add_scaling_menu_item(size_t scale);
                status_t            show_menu(tk::Widget *actor);
                status_t            bind_event(const char *id, tk::slot_t slot, tk::event_handler_t handler);
                status_t            init_ui_scaling();
                status_t            init_bundle_scaling();

            public:
                explicit FontScaling(ui::IWrapper *src);
                FontScaling(const FontScaling &) = delete;
                FontScaling(FontScaling &&) = delete;
                virtual ~FontScaling() override;

                FontScaling & operator = (const FontScaling &) = delete;
                FontScaling & operator = (FontScaling &&) = delete;

            public: // ui::IPortListener
                virtual void        notify(ui::IPort *port, size_t flags) override;

            public:
                status_t            init();
                void                destroy();

            public:
                void                sync_parameters();

                inline tk::Menu    *menu()                                          { return wMenu; }

                inline status_t     bind_zoom_in(const char *id, tk::slot_t slot)   { return bind_event(id, slot, slot_zoom_in);     }
                inline status_t     bind_zoom_out(const char *id, tk::slot_t slot)  { return bind_event(id, slot, slot_zoom_out);    }
                inline status_t     bind_show(const char *id, tk::slot_t slot)      { return bind_event(id, slot, slot_show);        }
        };


    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_FONTSCALING_H_ */
