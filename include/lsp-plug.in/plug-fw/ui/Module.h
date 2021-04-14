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
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>

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
            private:
                Module &operator = (const Module &);

            protected:
                const meta::plugin_t               *pMetadata;
                IWrapper                           *pWrapper;
                tk::Display                        *pDisplay;           // Display object
                tk::Widget                         *wRoot;              // Root widget (window)
                lltl::pphash<char, tk::Widget>      sMapping;
                lltl::parray<tk::Widget>            vWidgets;

            protected:
                void                        do_destroy();
                bool                        remove_item(lltl::parray<tk::Widget> *slist, tk::Widget *w);

            public:
                explicit Module(const meta::plugin_t *meta);
                virtual ~Module();

                /** Initialize UI
                 *
                 * @param wrapper plugin wrapper
                 * @param dpy display object
                 * @return status of operation
                 */
                virtual status_t            init(IWrapper *wrapper, tk::Display *dpy);

                /**
                 * Destroy the UI
                 */
                virtual void                destroy();

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
                void                        position_updated(const plug::position_t *pos);

                /**
                 * Synchronize state of meta ports
                 */
                void                        sync_meta_ports();

                /**
                 * Notify the write of the KVT parameter
                 * @param storage KVT storage
                 * @param id kvt parameter identifier
                 * @param value KVT parameter value
                 */
                virtual void                kvt_write(core::KVTStorage *storage, const char *id, const core::kvt_param_t *value);

                /**
                 * Add widget
                 * @param w widget to add
                 * @return status of operation
                 */
                status_t                    add_widget(tk::Widget *w);

                /**
                 * Map widget (assign unique identifier)
                 * @param uid unique identifier of widget
                 * @param w widget to map
                 * @return status of operation
                 */
                status_t                    map_widget(const char *uid, tk::Widget *w);
                status_t                    map_widget(const LSPString *uid, tk::Widget *w);

                /**
                 * Unmap widget by it's identifier
                 * @param uid unique widget identifier
                 * @return status of operation
                 */
                status_t                    unmap_widget(const char *uid);
                status_t                    unmap_widget(const LSPString *uid);

                /**
                 * Unmap widget by it's pointer to the instance
                 * @param w pointer to widget instance
                 * @return status of operation
                 */
                status_t                    unmap_widget(const tk::Widget *w);

                /**
                 * Unmap all widgets passed as arguments
                 * @param w pointer to array of widgets
                 * @param n size of array
                 * @return number of unmapped widgets or negative error code
                 */
                ssize_t                     unmap_widgets(const tk::Widget * const *w, size_t n);

                /**
                 * Map widget (assign unique identifier)
                 * @param uid unique identifier of widget
                 * @return the resolved widget or NULL
                 */
                tk::Widget                 *find_widget(const char *uid);
                tk::Widget                 *find_widget(const LSPString *uid);
        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_UI_MODULE_H_ */
