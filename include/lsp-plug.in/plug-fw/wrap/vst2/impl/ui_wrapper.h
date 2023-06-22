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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ui_ports.h>

namespace lsp
{
    namespace vst2
    {
        UIWrapper::UIWrapper(ui::Module *ui, vst2::Wrapper *wrapper):
            IWrapper(ui, wrapper->resources())
        {
            pWrapper        = wrapper;
            pIdleThread     = NULL;
            nKeyState       = 0;
            sRect.top       = 0;
            sRect.left      = 0;
            sRect.bottom    = 0;
            sRect.right     = 0;
        }

        UIWrapper::~UIWrapper()
        {
        }

        vst2::UIPort *UIWrapper::create_port(const meta::port_t *port, const char *postfix)
        {
            // Find the matching port for the backend
            vst2::UIPort *vup = NULL;
            vst2::Port *vp    = pWrapper->find_by_id(port->id);
            if (vp == NULL)
                return vup;

            switch (port->role)
            {
                case meta::R_AUDIO: // Stub port
                    lsp_trace("creating stub audio port %s", port->id);
                    vup = new vst2::UIPort(port, vp);
                    break;

                case meta::R_MESH:
                    lsp_trace("creating mesh port %s", port->id);
                    vup = new vst2::UIMeshPort(port, vp);
                    break;

                case meta::R_STREAM:
                    lsp_trace("creating stream port %s", port->id);
                    vup = new vst2::UIStreamPort(port, vp);
                    break;

                case meta::R_FBUFFER:
                    lsp_trace("creating fbuffer port %s", port->id);
                    vup = new vst2::UIFrameBufferPort(port, vp);
                    break;

                case meta::R_OSC:
                    lsp_trace("creating osc port %s", port->id);
                    if (meta::is_out_port(port))
                        vup     = new vst2::UIOscPortIn(port, vp);
                    else
                        vup     = new vst2::UIOscPortOut(port, vp);
                    break;

                case meta::R_PATH:
                    lsp_trace("creating path port %s", port->id);
                    vup = new vst2::UIPathPort(port, vp);
                    break;

                case meta::R_CONTROL:
                case meta::R_METER:
                case meta::R_BYPASS:
                    lsp_trace("creating regular port %s", port->id);
                    // VST specifies only INPUT parameters, output should be read in different way
                    if (meta::is_out_port(port))
                        vup     = new vst2::UIMeterPort(port, vp);
                    else
                        vup     = new vst2::UIParameterPort(port, static_cast<vst2::ParameterPort *>(vp));
                    break;

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES], param_name[MAX_PARAM_ID_BYTES];
                    lsp_trace("creating port group %s", port->id);
                    UIPortGroup *upg = new vst2::UIPortGroup(static_cast<vst2::PortGroup *>(vp));

                    // Add immediately port group to list
                    vPorts.add(upg);

                    // Add nested ports
                    lsp_trace("  rows = %d", int(upg->rows()));
                    for (size_t row=0; row < upg->rows(); ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Obtain the related port for backend
                        for (const meta::port_t *p = port->members; p->id != NULL; ++p)
                        {
                            // Initialize port name
                            strncpy(param_name, p->id, sizeof(param_name)-1);
                            strncat(param_name, postfix_buf, sizeof(param_name)-1);
                            param_name[sizeof(param_name) - 1] = '\0';

                            // Obtain backend port and create UI port for it
                            vp    = pWrapper->find_by_id(param_name);
                            lsp_trace("find_by_id %s -> %p", param_name, vp);
                            if (vp != NULL)
                                create_port(vp->metadata(), postfix_buf);
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            // Add port to the list of UI ports
            if (vup != NULL)
                vPorts.add(vup);

            return vup;
        }

        status_t UIWrapper::init(void *root_widget)
        {
            status_t res = STATUS_OK;

            // Obtain UI metadata
            const meta::plugin_t *meta = pUI->metadata();
            if (pUI->metadata() == NULL)
                return STATUS_BAD_STATE;

            // Create list of ports and sort it in ascending order by the identifier
            lsp_trace("Creating ports for %s - %s", meta->name, meta->description);
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(port, NULL);

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
                if ((res = build_ui(meta->ui_resource, root_widget)) != STATUS_OK)
                {
                    lsp_error("Error building UI for resource %s: code=%d", meta->ui_resource, int(res));
                    return res;
                }
            }

            // Bind resize slot
            tk::Window *wnd  = window();
            if (wnd != NULL)
            {
                wnd->slots()->bind(tk::SLOT_RESIZE, slot_ui_resize, this);
                wnd->slots()->bind(tk::SLOT_SHOW, slot_ui_show, this);
                wnd->slots()->bind(tk::SLOT_REALIZED, slot_ui_realize, this);
            }

            // Call the post-initialization routine
            if (res == STATUS_OK)
                res = pUI->post_init();

            return res;
        }

        void UIWrapper::destroy()
        {
            // Terminate idle thread
            terminate_idle_thread();

            // Destroy UI
            if (pUI != NULL)
            {
                pUI->pre_destroy();
                pUI->destroy();
                delete pUI;
                pUI = NULL;
            }

            // Call parent instance
            IWrapper::destroy();
        }

        void UIWrapper::terminate_idle_thread()
        {
            // Terminate idle thread if it is present
            if (pIdleThread == NULL)
                return;

            pIdleThread->cancel();
            pIdleThread->join();

            delete pIdleThread;
            pIdleThread     = NULL;
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
            pWrapper->request_state_dump();
        }

        void UIWrapper::main_iteration()
        {
            transfer_dsp_to_ui();

            IWrapper::main_iteration();

            // Call main iteration for the underlying display
            // For windows, we do not need to call main_iteration() because the main
            // event loop is provided by the hosting application
        #ifndef PLATFORM_WINDOWS
            if (pDisplay != NULL)
                pDisplay->main_iteration();
        #endif /* PLATFORM_WINDOWS */
        }

        const meta::package_t *UIWrapper::package() const
        {
            return pWrapper->package();
        }

        void UIWrapper::transfer_dsp_to_ui()
        {
            // Initialize DSP state
            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            // Try to sync position
            IWrapper::position_updated(pWrapper->position());

            // DSP -> UI communication
            for (size_t i=0, nports=vPorts.size(); i < nports; ++i)
            {
                // Get UI port
                vst2::UIPort *vup   = static_cast<vst2::UIPort *>(vPorts.uget(i));
                do {
                    if (vup->sync())
                        vup->notify_all();
                } while (vup->sync_again());
            } // for port_id

            // Perform KVT synchronization
            core::KVTStorage *kvt = pWrapper->kvt_lock();
            if (kvt != NULL)
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

                        kvt_dump_parameter("RX kvt param (UI->DSP): %s = ", kvt_value, kvt_name);
                    }
                }
                #else
                    kvt-> commit_all(core::KVT_RX);    // Just clear all RX queue for non-debug version
                #endif /* LSP_DEBUG */

                // Call garbage collection and release KVT storage
                kvt->gc();
                kvt_release();
            }

            // Notify sample listeners if something has changed
            core::SamplePlayer *sp = pWrapper->sample_player();
            if (sp != NULL)
                notify_play_position(sp->position(), sp->sample_length());
        }

        bool UIWrapper::show_ui()
        {
            // Reset key state
            nKeyState = 0;

            // Force all parameters to be re-shipped to the UI
            for (size_t i=0; i<vPorts.size(); ++i)
            {
                vst2::UIPort  *vp   = static_cast<vst2::UIPort *>(vPorts.uget(i));
                if (vp != NULL)
                    vp->notify_all();
            }

            core::KVTStorage *kvt = kvt_lock();
            if (kvt != NULL)
            {
                kvt->touch_all(core::KVT_TO_UI);
                kvt_release();
            }
            transfer_dsp_to_ui();

            // Show the UI window
            window()->show();

            // Launch the idle thread (workaround for some hosts)
            pIdleThread = new ipc::Thread(eff_edit_idle, this);
            if (pIdleThread == NULL)
                return false;
            pIdleThread->start();

            return true;
        }

        void UIWrapper::hide_ui()
        {
            tk::Window *wnd     = window();
            if (wnd != NULL)
                wnd->hide();
            terminate_idle_thread();
        }

        void UIWrapper::resize_ui()
        {
            tk::Window *wnd     = window();
            if ((wnd == NULL) || (!wnd->visibility()->get()))
                return;

            ws::rectangle_t rr;
            if (wnd->get_screen_rectangle(&rr) != STATUS_OK)
                return;

            lsp_trace("Get geometry: width=%d, height=%d", int(rr.nWidth), int(rr.nHeight));
            lsp_trace("audioMasterSizeWindow width=%d, height=%d", int(rr.nWidth), int(rr.nHeight));
            if (((sRect.right - sRect.left) != rr.nWidth) ||
                  ((sRect.bottom - sRect.top) != rr.nHeight))
            {
                pWrapper->pMaster(pWrapper->pEffect, audioMasterSizeWindow, rr.nWidth, rr.nHeight, 0, 0);
                sRect.right     = rr.nWidth;
                sRect.bottom    = rr.nHeight;
            }
        }

        void UIWrapper::idle_ui()
        {
            // Terminate the idle thread if it has been launched previously
            terminate_idle_thread();

            // Call the processing of main iteration
            main_iteration();
        }

        ERect *UIWrapper::ui_rect()
        {
            lsp_trace("left=%d, top=%d, right=%d, bottom=%d",
                    int(sRect.left), int(sRect.top), int(sRect.right), int(sRect.bottom)
                );
            return &sRect;
        }

        status_t UIWrapper::slot_ui_resize(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *wrapper = static_cast<UIWrapper *>(ptr);
            wrapper->resize_ui();
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_show(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *wrapper = static_cast<UIWrapper *>(ptr);
            wrapper->resize_ui();
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_realize(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *wrapper = static_cast<UIWrapper *>(ptr);
            wrapper->resize_ui();
            return STATUS_OK;
        }

        size_t UIWrapper::key_state() const
        {
            return nKeyState;
        }

        size_t UIWrapper::set_key_state(size_t state)
        {
            return nKeyState = state;
        }

        status_t UIWrapper::eff_edit_idle(void *arg)
        {
            static constexpr size_t FRAME_PERIOD    = 1000 / UI_FRAMES_PER_SECOND;

            UIWrapper *this_ = static_cast<UIWrapper *>(arg);
            system::time_millis_t ctime = system::get_time_millis();

            while (!ipc::Thread::is_cancelled())
            {
                // Measure the time of next frame to appear
                system::time_millis_t deadline = ctime + FRAME_PERIOD;

                // Perform main iteration
                this_->main_iteration();

                // Wait for the next frame to appear
                system::time_millis_t ftime = system::get_time_millis();
                if (ftime < deadline)
                    this_->pDisplay->wait_events(deadline - ftime);
                ctime   = ftime;
            }

            return STATUS_OK;
        }

        UIWrapper *UIWrapper::create(vst2::Wrapper *wrapper, void *root_widget)
        {
            const char *vst2_uid = wrapper->metadata()->vst2_uid;

            // Lookup plugin identifier among all registered plugin factories
            for (ui::Factory *f = ui::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Check plugin identifier
                    if (!::strcmp(meta->vst2_uid, vst2_uid))
                    {
                        // Instantiate the plugin UI and return
                        ui::Module *ui = f->create(meta);
                        vst2::UIWrapper *ui_wrapper = (ui != NULL) ? new vst2::UIWrapper(ui, wrapper) : NULL;

                        if (ui_wrapper != NULL)
                        {
                            if (ui_wrapper->init(root_widget) == STATUS_OK)
                                return ui_wrapper;

                            ui_wrapper->destroy();
                            delete wrapper;
                        }
                        else if (ui != NULL)
                        {
                            ui->destroy();
                            delete ui;
                        }

                        return NULL;
                    }
                }
            }

            // No plugin has been found
            fprintf(stderr, "Not found UI for plugin: %s, will continue in headless mode\n", vst2_uid);
            return NULL;
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
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_UI_WRAPPER_H_ */
