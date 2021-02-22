/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 янв. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>

#include <lsp-plug.in/plug-fw/wrap/jack/ui_ports.h>
#include <lsp-plug.in/plug-fw/wrap/jack/wrapper.h>
#include <lsp-plug.in/lltl/parray.h>

namespace lsp
{
    namespace jack
    {
        /**
         * UI wrapper for JACK
         */
        class UIWrapper: public ui::IWrapper
        {
            protected:
                plug::Module       *pPlugin;
                jack::Wrapper      *pWrapper;

                atomic_t                        nPosition;          // Position counter
                plug::position_t                sPosition;          // Actual time position

                lltl::parray<jack::UIPort>      vPorts;             // All ports
                lltl::parray<jack::UIPort>      vSyncPorts;         // Ports for synchronization
                lltl::parray<meta::port_t>      vGenMetadata;       // Generated metadata for virtual ports

            public:
                explicit UIWrapper(jack::Wrapper *wrapper, ui::Module *ui);
                virtual ~UIWrapper();

                virtual status_t init();
                virtual void destroy();

            protected:
                status_t    create_port(const meta::port_t *port, const char *postfix);
                void        sync_kvt(core::KVTStorage *kvt);

            public:
                virtual core::KVTStorage *kvt_lock();

                virtual core::KVTStorage *kvt_trylock();

                virtual bool kvt_release();

                virtual void dump_state_request();

            public:
                /**
                 * Perform single iteration of main event loop of the plugin
                 */
                void        main_iteration();

                /**
                 * Transfer all desired data from DSP to UI
                 * @param ts current execution timestamp
                 */
                bool        sync(ws::timestamp_t ts);
        };

        // Implementation
        UIWrapper::UIWrapper(jack::Wrapper *wrapper, ui::Module *ui) : ui::IWrapper(ui)
        {
            pPlugin     = wrapper->pPlugin;
            pWrapper    = wrapper;
            nPosition   = 0;

            plug::position_t::init(&sPosition);
        }

        UIWrapper::~UIWrapper()
        {
            pPlugin     = NULL;
            pWrapper    = NULL;
        }

        status_t UIWrapper::init()
        {
            status_t res = STATUS_OK;

            // Force position sync at startup
            nPosition   = pWrapper->nPosition - 1;

            // Create list of ports and sort it in ascending order
            for (const meta::port_t *meta = pUI->metadata()->ports ; meta->id != NULL; ++meta) {
                if ((res = create_port(meta, NULL)) != STATUS_OK)
                    return res;
            }

            return res;
        }

        void UIWrapper::destroy()
        {
            // Destroy ports
            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                jack::UIPort *p = vPorts.uget(i);
                lsp_trace("destroy UI port id=%s", p->metadata()->id);
                delete p;
            }
            vPorts.flush();
            vSyncPorts.flush();

            // Cleanup generated metadata
            for (size_t i=0, n=vGenMetadata.size(); i<n; ++i)
            {
                meta::port_t *port = vGenMetadata.uget(i);
                lsp_trace("destroy generated UI port metadata %p", port);
                meta::drop_port_metadata(port);
            }
        }

        core::KVTStorage *UIWrapper::kvt_lock()
        {
            return pWrapper->kvt_lock();
        }

        core::KVTStorage *UIWrapper::kvt_trylock()
        {
            return pWrapper->kvt_trylock();
        }

        bool UIWrapper::kvt_release()
        {
            return pWrapper->kvt_release();
        }

        void UIWrapper::dump_state_request()
        {
            return pWrapper->dump_plugin_state();
        }

        status_t UIWrapper::create_port(const meta::port_t *port, const char *postfix)
        {
            // Find the matching port for the backend
            jack::Port *jp    = pWrapper->port_by_id(port->id);
            if (jp == NULL)
                return STATUS_OK;

            // Create UI port
            jack::UIPort *jup = NULL;

            switch (port->role)
            {
                case meta::R_MESH:
                    jup     = new jack::UIMeshPort(jp);
                    if (meta::is_out_port(port))
                        vSyncPorts.add(jup);
                    break;

                case meta::R_FBUFFER:
                    jup     = new jack::UIFrameBufferPort(jp);
                    if (meta::is_out_port(port))
                        vSyncPorts.add(jup);
                    break;

                case meta::R_OSC:
                    if (meta::is_out_port(port))
                    {
                        jup     = new jack::UIOscPortIn(jp);
                        vSyncPorts.add(jup);
                    }
                    else
                        jup     = new jack::UIOscPortOut(jp);
                    break;

                case meta::R_PATH:
                    jup     = new jack::UIPathPort(jp);
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                    jup     = new jack::UIControlPort(jp);
                    break;

                case meta::R_METER:
                    jup     = new jack::UIMeterPort(jp);
                    vSyncPorts.add(jup);
                    break;

                case meta::R_PORT_SET:
                {
                    LSPString postfix_str;
                    jack::PortGroup *pg     = static_cast<jack::PortGroup *>(jp);
                    jack::UIPortGroup *upg  = new jack::UIPortGroup(pg);
                    vPorts.add(upg);
                    pUI->add_port(upg);

                    for (size_t row=0; row<pg->rows(); ++row)
                    {
                        // Generate postfix
                        postfix_str.fmt_ascii("%s_%d", (postfix != NULL) ? postfix : "", int(row));
                        postfix = postfix_str.get_ascii();

                        // Clone port metadata
                        meta::port_t *cm        = clone_port_metadata(port->members, postfix);
                        if (cm != NULL)
                        {
                            vGenMetadata.add(cm);

                            for (; cm->id != NULL; ++cm)
                            {
                                if (meta::is_growing_port(cm))
                                    cm->start    = cm->min + ((cm->max - cm->min) * row) / float(pg->rows());
                                else if (meta::is_lowering_port(cm))
                                    cm->start    = cm->max - ((cm->max - cm->min) * row) / float(pg->rows());

                                create_port(cm, postfix);
                            }
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            if (jup != NULL)
            {
                vPorts.add(jup);
                pUI->add_port(jup);
            }

            return STATUS_OK;
        }

        void UIWrapper::main_iteration()
        {
            // TODO
        }

        void UIWrapper::sync_kvt(core::KVTStorage *kvt)
        {
            // Synchronize DSP -> UI transfer
            size_t sync;
            const char *kvt_name;
            const core::kvt_param_t *kvt_value;

            do
            {
                sync = 0;

                core::KVTIterator *it = kvt->enum_tx_pending();
                while (it->next() == STATUS_OK)
                {
                    kvt_name = it->name();
                    if (kvt_name == NULL)
                        break;
                    status_t res = it->get(&kvt_value);
                    if (res != STATUS_OK)
                        break;
                    if ((res = it->commit(core::KVT_TX)) != STATUS_OK)
                        break;

                    kvt_dump_parameter("TX kvt param (DSP->UI): %s = ", kvt_value, kvt_name);
                    pUI->kvt_write(kvt, kvt_name, kvt_value);
                    ++sync;
                }
            } while (sync > 0);

            // Synchronize UI -> DSP transfer
            #ifdef LSP_DEBUG
            {
                core::KVTIterator *it = kvt->enum_rx_pending();
                while (it->next() == STATUS_OK)
                {
                    kvt_name = it->name();
                    if (kvt_name == NULL)
                        break;
                    status_t res = it->get(&kvt_value);
                    if (res != STATUS_OK)
                        break;
                    if ((res = it->commit(core::KVT_RX)) != STATUS_OK)
                        break;

                    core::kvt_dump_parameter("RX kvt param (UI->DSP): %s = ", kvt_value, kvt_name);
                }
            }
            #else
                kvt->commit_all(core::KVT_RX);    // Just clear all RX queue for non-debug version
            #endif
        }

        bool UIWrapper::sync(ws::timestamp_t ts)
        {
            // Validate state
            dsp::context_t ctx;
            dsp::start(&ctx);

            // Check that position has been updated and sync it's state
            atomic_t pos    = pWrapper->nPosition;
            if (nPosition != pos)
            {
                sPosition       = pWrapper->sPosition;
                pUI->position_updated(&sPosition);
                nPosition       = pos;
            }

            // Transfer the values of the ports to the UI
            size_t sync = vSyncPorts.size();
            for (size_t i=0; i<sync; ++i)
            {
                jack::UIPort *jup   = vSyncPorts.uget(i);
                do {
                    if (jup->sync())
                        jup->notify_all();
                } while (jup->sync_again());
            }

            // TODO: Transfer data for meta ports
            // pUI->sync_meta_ports();

            // Synchronize KVT state
            core::KVTStorage *kvt = pWrapper->kvt_trylock();
            if (kvt != NULL)
            {
                sync_kvt(kvt);

                // Call garbage collection and release KVT storage
                kvt->gc();
                pWrapper->kvt_release();
            }

            // Limit refresh rate of window icon and refresh icon
//            if (nCounter++ >= 5)
//            {
//                // Reset counter first
//                nCounter = 0;
//                LSPWindow *root = (pUI != NULL) ? pUI->root_window() : NULL;
//
//                if (root != NULL)
//                {
//                    // Check if inline display is present
//                    canvas_data_t *data = NULL;
//                    if (test_display_draw())
//                        data = render_inline_display(JACK_INLINE_DISPLAY_SIZE, JACK_INLINE_DISPLAY_SIZE);
//
//                    // Check that returned data is valid
//                    if ((data != NULL) && (data->pData != NULL) && (data->nWidth > 0) && (data->nHeight > 0))
//                    {
//                        size_t row_size = data->nWidth * sizeof(uint32_t);
//
//                        if (data->nStride > row_size)
//                        {
//                            // Compress image data if stride is greater than row size
//                            uint8_t *dst = data->pData;
//                            for (size_t row = 0; row < data->nHeight; ++row)
//                            {
//                                uint8_t *p  = &data->pData[row * data->nStride];
//                                memmove(dst, p, row_size);
//                            }
//                        }
//
//                        root->set_icon(data->pData, data->nWidth, data->nHeight);
//                    }
//                } // root != NULL
//            }

            dsp::finish(&ctx);

            return true;
        }
    }
}


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_WRAPPER_H_ */
