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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UISCALING_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UISCALING_H_

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
         * The controller for the UI scaling
         */
        class UIScaling: public ui::IPortListener
        {
            protected:
                typedef struct scaling_sel_t
                {
                    ctl::UIScaling     *ctl;
                    float               scaling;
                    tk::MenuItem       *item;
                } scaling_sel_t;

            protected:
                ui::IWrapper       *pWrapper;           // Plugin wrapper
                tk::Menu           *wUIScaling;         // UI scaling menu
                tk::Menu           *wBundleScaling;     // Bundle Scaling menu
                tk::MenuItem       *wPreferHost;        // 'Prefer host' menu item

                ui::IPort          *pUIScaling;
                ui::IPort          *pUIScalingHost;
                ui::IPort          *pBundleScaling;

                lltl::parray<scaling_sel_t> vUIScalingSel;
                lltl::parray<scaling_sel_t> vBundleScalingSel;

            protected: // Slots
                static status_t     slot_ui_scaling_prefer_host(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_ui_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_ui_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_ui_scaling_select(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_ui_scaling_show(tk::Widget *sender, void *ptr, void *data);

                static status_t     slot_bundle_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_bundle_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_bundle_scaling_select(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_bundle_scaling_show(tk::Widget *sender, void *ptr, void *data);

            protected:
                tk::MenuItem       *create_menu_item(tk::Menu *dst);
                tk::Menu           *create_menu();
                status_t            add_scaling_menu_item(
                    lltl::parray<scaling_sel_t> & list,
                    tk::Menu *menu, const char *key, size_t scale,
                    tk::event_handler_t handler);
                status_t            show_menu(tk::Menu *menu, tk::Widget *actor);
                status_t            bind_event(const char *id, tk::slot_t slot, tk::event_handler_t handler);
                status_t            init_ui_scaling();
                status_t            init_bundle_scaling();

            public:
                explicit UIScaling(ui::IWrapper *src);
                UIScaling(const UIScaling &) = delete;
                UIScaling(UIScaling &&) = delete;
                virtual ~UIScaling() override;

                UIScaling & operator = (const UIScaling &) = delete;
                UIScaling & operator = (UIScaling &&) = delete;

            public: // ui::IPortListener
                virtual void        notify(ui::IPort *port, size_t flags) override;

            public:
                status_t            init(bool bundle_scaling);
                void                destroy();

            public:
                ssize_t             get_bundle_scaling();
                void                host_scaling_changed();
                void                sync_parameters();

                inline tk::Menu    *ui_scaling_menu()                               { return wUIScaling; }
                inline tk::Menu    *bundle_scaling_menu()                           { return wBundleScaling; }

                inline status_t     bind_ui_scaling_zoom_in(const char *id, tk::slot_t slot)   { return bind_event(id, slot, slot_ui_scaling_zoom_in);    }
                inline status_t     bind_ui_scaling_zoom_out(const char *id, tk::slot_t slot)  { return bind_event(id, slot, slot_ui_scaling_zoom_out);   }
                inline status_t     bind_ui_scaling_show(const char *id, tk::slot_t slot)      { return bind_event(id, slot, slot_ui_scaling_show);          }

                inline status_t     bind_bundle_scaling_zoom_in(const char *id, tk::slot_t slot)   { return bind_event(id, slot, slot_bundle_scaling_zoom_in);    }
                inline status_t     bind_bundle_scaling_zoom_out(const char *id, tk::slot_t slot)  { return bind_event(id, slot, slot_bundle_scaling_zoom_out);   }
                inline status_t     bind_bundle_scaling_show(const char *id, tk::slot_t slot)      { return bind_event(id, slot, slot_bundle_scaling_show);       }
        };


    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UISCALING_H_ */
