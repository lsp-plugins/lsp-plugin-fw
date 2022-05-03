/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ui_ports.h>

namespace lsp
{
    namespace vst2
    {
        class Wrapper;

        class UIWrapper: public ui::IWrapper
        {
            private:
                vst2::Wrapper                      *pWrapper;       // VST Wrapper
                size_t                              nKeyState;      // State of the keys
                ERect                               sRect;

            protected:
                static status_t slot_ui_resize(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_ui_show(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_ui_realize(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                            transfer_dsp_to_ui();
                vst2::UIPort                   *create_port(const meta::port_t *port, const char *postfix);

            public:
                explicit UIWrapper(ui::Module *ui, vst2::Wrapper *wrapper);
                virtual ~UIWrapper();

                virtual status_t                init(void *root_widget);
                virtual void                    destroy();

            public:
                virtual core::KVTStorage       *kvt_lock();
                virtual core::KVTStorage       *kvt_trylock();
                virtual bool                    kvt_release();

                virtual void                    dump_state_request();
                virtual void                    main_iteration();

                virtual const meta::package_t  *package() const;

            public:
                bool                            show_ui();
                void                            hide_ui();
                void                            resize_ui();
                ERect                          *ui_rect();
                size_t                          key_state() const;
                size_t                          set_key_state(size_t state);

            public:
                static UIWrapper               *create(vst2::Wrapper *wrapper, void *root_widget);
        };
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_WRAPPER_H_ */
