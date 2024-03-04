/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_MODULE_H_
#define LSP_PLUG_IN_PLUG_FW_UI_MODULE_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/ui/IWrapper.h>

#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ui
    {
        class IWrapper;

        /**
         * UI Module
         */
        class Module
        {
            protected:
                const meta::plugin_t               *pMetadata;
                IWrapper                           *pWrapper;
                tk::Display                        *pDisplay;           // Display object
                tk::Widget                         *wRoot;              // Root widget (window)

            protected:
                void                            do_destroy();

            protected:
                ui::IPort                      *create_control_port(const meta::port_t *meta);
                status_t                        add_custom_port(ui::IPort *port);

            public:
                explicit Module(const meta::plugin_t *meta);
                Module(const Module &) = delete;
                Module(Module &&) = delete;
                virtual ~Module();

                Module &operator = (const Module &) = delete;
                Module &operator = (Module &&) = delete;

                /** Initialize UI
                 *
                 * @param wrapper plugin wrapper
                 * @param dpy display object
                 * @return status of operation
                 */
                virtual status_t                init(IWrapper *wrapper, tk::Display *dpy);

                /**
                 * Destroy the UI
                 */
                virtual void                    destroy();

                /**
                 * Method is called after all UI initialization has been completed
                 * @param window the actual main window of the plugin
                 * @return status of operation
                 */
                virtual status_t                post_init();

                /**
                 * Method is called before the UI destruction is performed
                 * @return status of operation
                 */
                virtual status_t                pre_destroy();

                /**
                 * Method is called when the plugin wrapper triggers the plugin reset event.
                 * This method can apply additional plugin state updates, for example, change
                 * the contents of the KVT storage.
                 * @return status of operation
                 */
                virtual status_t                reset_settings();

                /**
                 * This method is called on each UI iteration.
                 */
                virtual void                    idle();

                /**
                 * Triggers when any KVT parameter change occurs
                 * @param kvt KVT storage
                 * @param id KVT parameter identifier
                 * @param value KVT parameter value
                 */
                virtual void                    kvt_changed(core::KVTStorage *kvt, const char *id, const core::kvt_param_t *value);

            public:
                inline const meta::plugin_t    *metadata() const        { return pMetadata;         }
                inline IWrapper                *wrapper()               { return pWrapper;          }
                inline tk::Display             *display()               { return pDisplay;          }
                inline tk::Widget              *root()                  { return wRoot;             }

                inline void                     set_root(tk::Widget *root)  { wRoot = root;         }

            public:
                /** Method executed when the time position of plugin was updated
                 *
                 */
                void                            position_updated(const plug::position_t *pos);
        };

    } /* namespace ui */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UI_MODULE_H_ */
