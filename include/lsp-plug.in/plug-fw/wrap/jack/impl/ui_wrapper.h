/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 31 янв. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/wrap/jack/ui_wrapper.h>

#define JACK_STATUS_OFF         "PluginWindow::StatusBar::Label::FAIL"
#define JACK_STATUS_ON          "PluginWindow::StatusBar::Label::OK"

namespace lsp
{
    namespace jack
    {
        UIWrapper::UIWrapper(jack::Wrapper *wrapper, resource::ILoader *loader, ui::Module *ui) : ui::IWrapper(ui, loader)
        {
            pPlugin         = wrapper->pPlugin;
            pWrapper        = wrapper;

            nPosition       = 0;
            pJackStatus     = NULL;
            bJackConnected  = false;
        }

        UIWrapper::~UIWrapper()
        {
            do_destroy();
        }

        status_t UIWrapper::init(void *root_widget)
        {
            status_t res = STATUS_OK;

            // Force position sync at startup
            nPosition   = pWrapper->nPosition - 1;
            const meta::plugin_t *meta = pUI->metadata();
            if (pUI->metadata() == NULL)
                return STATUS_BAD_STATE;

            // Create list of ports and sort it in ascending order by the identifier
            lsp_trace("Creating ports for %s - %s", meta->name, meta->description);
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port) {
                if ((res = create_port(port, NULL)) != STATUS_OK)
                    return res;
            }

            // Initialize parent
            if ((res = IWrapper::init(root_widget)) != STATUS_OK)
                return res;

            // Initialize display settings
            tk::display_settings_t settings;
            resource::Environment env;

            settings.resources      = pLoader;
            settings.environment    = &env;

            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_DICT_PATH, LSP_BUILTIN_PREFIX "i18n"));
            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_LANG, "us"));
            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_CONFIG, "lsp-plugins"));

            // Create the display
            pDisplay = new tk::Display(&settings);
            if (pDisplay == NULL)
                return STATUS_NO_MEM;
            if ((res = pDisplay->init(0, NULL)) != STATUS_OK)
                return res;

            // Load visual schema
            if ((res = init_visual_schema()) != STATUS_OK)
                return res;

            // Initialize the UI
            if ((res = pUI->init(this, pDisplay)) != STATUS_OK)
                return res;

            // Build the UI
            if (meta->ui_resource != NULL)
            {
                if ((res = build_ui(meta->ui_resource)) != STATUS_OK)
                {
                    lsp_error("Error building UI for resource %s: code=%d", meta->ui_resource, int(res));
                    return res;
                }
            }

            // Call the post-initialization routine and show the 'jack' state indicator
            if ((res = pUI->post_init()) == STATUS_OK)
            {
                pJackStatus = tk::widget_cast<tk::Label>(controller()->widgets()->find("jack_status"));
                if (pJackStatus != NULL)
                {
                    tk::Widget *w = controller()->widgets()->find("jack_indicator");
                    if (w != NULL)
                    {
                        w->visibility()->set(true);
                        set_connection_status(bJackConnected);
                    }
                }
            }

            tk::Window *root    = window();
            if (root == NULL)
            {
                lsp_error("No root window present!\n");
                return STATUS_BAD_STATE;
            }

            // Bind events to the root window
            root->slot(tk::SLOT_SHOW)->bind(slot_ui_show, this);
            root->slot(tk::SLOT_HIDE)->bind(slot_ui_hide, this);

            return res;
        }

        void UIWrapper::do_destroy()
        {
            pJackStatus = NULL;

            // Call the parent class for destroy
            IWrapper::destroy();

            // Destroy ports
            vSyncPorts.flush();

            // Cleanup generated metadata
            for (size_t i=0, n=vGenMetadata.size(); i<n; ++i)
            {
                meta::port_t *port = vGenMetadata.uget(i);
                lsp_trace("destroy generated UI port metadata %p", port);
                meta::drop_port_metadata(port);
            }
            vGenMetadata.flush();

            // Destroy the display
            if (pDisplay != NULL)
            {
                pDisplay->destroy();
                delete pDisplay;
                pDisplay    = NULL;
            }

            pPlugin         = NULL;
            pWrapper        = NULL;
        }

        void UIWrapper::destroy()
        {
            do_destroy();
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

        const meta::package_t *UIWrapper::package() const
        {
            return pWrapper->package();
        }

        meta::plugin_format_t UIWrapper::plugin_format() const
        {
            return meta::PLUGIN_JACK;
        }

        void UIWrapper::dump_state_request()
        {
            return pWrapper->dump_plugin_state();
        }

        void UIWrapper::main_iteration()
        {
            IWrapper::main_iteration();

            // Always call main iteration for the underlying display
            if (pDisplay != NULL)
                pDisplay->main_iteration();
        }

        status_t UIWrapper::play_file(const char *file, wsize_t position, bool release)
        {
            core::SamplePlayer *p = pWrapper->sample_player();
            if (p != NULL)
            {
                // Trigger playback and force the position to become out-of-sync
                p->play_sample(file, position, release);
            }
            return STATUS_OK;
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
                case meta::R_AUDIO_IN:
                case meta::R_AUDIO_OUT:
                    // Stub port
                    jup     = new jack::UIPort(jp);
                    break;

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

                case meta::R_STREAM:
                    jup     = new jack::UIStreamPort(jp);
                    if (meta::is_out_port(port))
                        vSyncPorts.add(jup);
                    break;

                case meta::R_OSC_OUT:
                    jup     = new jack::UIOscPortIn(jp);
                    vSyncPorts.add(jup);
                    break;
                case meta::R_OSC_IN:
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

                    for (size_t row=0; row<pg->rows(); ++row)
                    {
                        // Generate postfix
                        postfix_str.fmt_ascii("%s_%d", (postfix != NULL) ? postfix : "", int(row));
                        const char *port_post = postfix_str.get_ascii();

                        // Clone port metadata
                        meta::port_t *cm        = clone_port_metadata(port->members, port_post);
                        if (cm != NULL)
                        {
                            vGenMetadata.add(cm);

                            for (; cm->id != NULL; ++cm)
                            {
                                if (meta::is_growing_port(cm))
                                    cm->start    = cm->min + ((cm->max - cm->min) * row) / float(pg->rows());
                                else if (meta::is_lowering_port(cm))
                                    cm->start    = cm->max - ((cm->max - cm->min) * row) / float(pg->rows());

                                create_port(cm, port_post);
                            }
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            if (jup != NULL)
                vPorts.add(jup);

            return STATUS_OK;
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
                    kvt_notify_write(kvt, kvt_name, kvt_value);
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
            // Update the state of connection
            if (!bJackConnected)
                set_connection_status(bJackConnected = true);

            // Initialize DSP state
            dsp::context_t ctx;
            dsp::start(&ctx);

            // Check that position has been updated and sync it's state
            atomic_t pos    = pWrapper->nPosition;
            if (nPosition != pos)
            {
                position_updated(pWrapper->position());
                nPosition       = pos;
            }

            // Transfer the values of the ports to the UI
            size_t sync = vSyncPorts.size();
            for (size_t i=0; i<sync; ++i)
            {
                jack::UIPort *jup   = vSyncPorts.uget(i);
                do {
                    if (jup->sync())
                        jup->notify_all(ui::PORT_NONE);
                } while (jup->sync_again());
            }

            // Synchronize KVT state
            core::KVTStorage *kvt = pWrapper->kvt_trylock();
            if (kvt != NULL)
            {
                sync_kvt(kvt);

                // Call garbage collection and release KVT storage
                kvt->gc();
                pWrapper->kvt_release();
            }

            // Notify sample listeners if something has changed
            core::SamplePlayer *sp = pWrapper->sample_player();
            if (sp != NULL)
                notify_play_position(sp->position(), sp->sample_length());

            dsp::finish(&ctx);

            return true;
        }

        void UIWrapper::sync_inline_display()
        {
            // Check that window is present
            if (wWindow == NULL)
                return;

            // Initialize DSP state
            dsp::context_t ctx;
            dsp::start(&ctx);

            // Check if inline display is present
            plug::canvas_data_t *data = NULL;
            if (pWrapper->test_display_draw())
                data = pWrapper->render_inline_display(JACK_INLINE_DISPLAY_SIZE, JACK_INLINE_DISPLAY_SIZE);

            // Check that returned data is valid
            if ((data != NULL) && (data->pData != NULL) && (data->nWidth > 0) && (data->nHeight > 0))
            {
                size_t row_size = data->nWidth * sizeof(uint32_t);

                if (data->nStride > row_size)
                {
                    // Compress image data if stride is greater than row size
                    uint8_t *dst = data->pData;
                    for (size_t row = 0; row < data->nHeight; ++row)
                    {
                        uint8_t *p  = &data->pData[row * data->nStride];
                        memmove(dst, p, row_size);
                    }
                }

                wWindow->set_icon(data->pData, data->nWidth, data->nHeight);
            }

            dsp::finish(&ctx);
        }

        status_t UIWrapper::slot_ui_hide(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *_this = static_cast<UIWrapper *>(ptr);
            if (_this->pWrapper != NULL)
                _this->pWrapper->set_ui_active(false);
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_show(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *_this = static_cast<UIWrapper *>(ptr);
            if (_this->pWrapper != NULL)
                _this->pWrapper->set_ui_active(true);
            return STATUS_OK;
        }

        void UIWrapper::connection_lost()
        {
            if (bJackConnected)
                set_connection_status(bJackConnected = false);
        }

        void UIWrapper::set_connection_status(bool connected)
        {
            if (!pJackStatus)
                return;

            ctl::revoke_style(pJackStatus, JACK_STATUS_OFF);
            ctl::revoke_style(pJackStatus, JACK_STATUS_ON);
            ctl::inject_style(pJackStatus, (connected) ? JACK_STATUS_ON : JACK_STATUS_OFF);
            pJackStatus->text()->set((connected) ? "statuses.jack.on" : "statuses.jack.off");
        }

        status_t UIWrapper::export_settings(config::Serializer *s, const io::Path *basedir)
        {
            // Notify the plugin the state is about to be saved
            pPlugin->before_state_save();

            // Synchronize KVT state as before_state_save() can change it
            core::KVTStorage *kvt = pWrapper->kvt_trylock();
            if (kvt != NULL)
            {
                lsp_finally { pWrapper->kvt_release(); };
                sync_kvt(kvt);
                kvt->gc();
            }

            // Do the usual stuff
            status_t res = ui::IWrapper::export_settings(s, basedir);

            // Notify the plugin that the state has been just saved
            if (res == STATUS_OK)
                pPlugin->state_saved();

            return res;
        }

        status_t UIWrapper::import_settings(config::PullParser *parser, size_t flags, const io::Path *basedir)
        {
            // Notify the plugin the state is about to be load
            pPlugin->before_state_load();

            // Do the usual stuff
            status_t res = ui::IWrapper::import_settings(parser, flags, basedir);

            // Synchronize KVT state as there can be changes
            core::KVTStorage *kvt = pWrapper->kvt_trylock();
            if (kvt != NULL)
            {
                lsp_finally { pWrapper->kvt_release(); };
                sync_kvt(kvt);
                kvt->gc();
            }

            // Notify the plugin that the state has been just loaded
            if (res == STATUS_OK)
            {
                pPlugin->state_loaded();
                pWrapper->bUpdateSettings = true;
            }

            return res;
        }
    } /* namespace jack */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_UI_WRAPPER_H_ */
