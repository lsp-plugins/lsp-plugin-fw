/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 5 янв. 2023 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/wrap/clap/wrapper.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace clap
    {
        UIWrapper::UIWrapper(ui::Module *ui, clap::Wrapper *wrapper):
            ui::IWrapper(ui, wrapper->resources())
        {
            pWrapper        = wrapper;
            pExt            = wrapper->extensions();
            fScaling        = -100.0f;
            pParent         = NULL;
            pTransientFor   = NULL;
            bUIInitialized  = false;
            bRequestProcess = false;
            bUIActive       = false;

        #ifdef LSP_CLAP_OWN_EVENT_LOOP
            pUIThread       = NULL;
        #endif /* LSP_CLAP_OWN_EVENT_LOOP */
        }

        UIWrapper::~UIWrapper()
        {
            do_destroy();
        }

        status_t UIWrapper::init(void *root_widget)
        {
            lsp_trace("Initializing wrapper");

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

            // Bind the display idle handler
            pDisplay->slots()->bind(tk::SLOT_IDLE, slot_display_idle, this);
            pDisplay->set_idle_interval(1000 / UI_FRAMES_PER_SECOND);

            // Load visual schema
            if ((res = init_visual_schema()) != STATUS_OK)
                return res;

            // Initialize the UI
            if ((res = pUI->init(this, pDisplay)) != STATUS_OK)
                return res;

            return res;
        }

        void UIWrapper::destroy()
        {
            do_destroy();
        }

        void UIWrapper::do_destroy()
        {
            // Update UI status
            bUIActive       = false;
            if (pWrapper != NULL)
                pWrapper->ui_visibility_changed();

            // Stop the event loop
            stop_event_loop();

            // Cleanup UI initialized flag
            bUIInitialized  = false;

            // Destroy the transient window wrapper
            if (pTransientFor != NULL)
            {
                delete pTransientFor;
                pTransientFor   = NULL;
            }

            // Stop the UI thread
            hide();

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

            // Destroy the display
            if (pDisplay != NULL)
            {
                pDisplay->destroy();
                delete pDisplay;
                pDisplay        = NULL;
            }
        }

        bool UIWrapper::start_event_loop()
        {
        #ifdef LSP_CLAP_OWN_EVENT_LOOP
            // Launch the main loop thread
            lsp_trace("Creating main loop thread");
            pUIThread   = new ipc::Thread(event_loop, this);
            if (pUIThread == NULL)
            {
                lsp_error("Failed to create UI main loop thread");
                return false;
            }

            if (pUIThread->start() != STATUS_OK)
            {
                lsp_error("Failed to start UI main loop thread");
                delete pUIThread;
                pUIThread = NULL;
                return false;
            }

            lsp_trace("Successfully started main loop thread");
        #endif /* LSP_CLAP_OWN_EVENT_LOOP */

            return true;
        }

        void UIWrapper::stop_event_loop()
        {
        #ifdef LSP_CLAP_OWN_EVENT_LOOP
            // Cancel the main loop thread
            if (pUIThread != NULL)
            {
                if (pDisplay != NULL)
                    pDisplay->quit_main();

                pUIThread->cancel();
                pUIThread->join();

                delete pUIThread;
                pUIThread = NULL;
            }
        #endif /* LSP_CLAP_OWN_EVENT_LOOP */
        }

    #ifdef LSP_CLAP_OWN_EVENT_LOOP
        status_t UIWrapper::event_loop(void *arg)
        {
            static constexpr size_t FRAME_PERIOD    = 1000 / UI_FRAMES_PER_SECOND;

            UIWrapper *self = static_cast<UIWrapper *>(arg);

            lsp_trace("Entering main loop");

            // Perform main loop
            system::time_millis_t ctime = system::get_time_millis();
            while (!ipc::Thread::is_cancelled())
            {
                // Measure the time of next frame to appear
                system::time_millis_t deadline = ctime + FRAME_PERIOD;

                // Perform main iteration with locked mutex
                if (self->sMutex.lock())
                {
                    lsp_finally { self->sMutex.unlock(); };
                    self->pDisplay->main_iteration();
                }

                // Wait for the next frame to appear
                system::time_millis_t ftime = system::get_time_millis();
                if (ftime < deadline)
                    self->pDisplay->wait_events(deadline - ftime);
                ctime   = ftime;
            }

            lsp_trace("Leaving main loop");

            return STATUS_OK;
        }
    #endif /* LSP_CLAP_OWN_EVENT_LOOP */

        clap::UIPort *UIWrapper::create_port(const meta::port_t *port, const char *postfix)
        {
            // Find the matching port for the backend
            clap::UIPort *cup = NULL;
            clap::Port *cp    = pWrapper->find_by_id(port->id);
            if (cp == NULL)
                return cup;

            switch (port->role)
            {
                case meta::R_AUDIO: // Stub port
                    lsp_trace("creating stub audio port %s", port->id);
                    cup = new clap::UIPort(port, cp);
                    break;

                case meta::R_MESH:
                    lsp_trace("creating mesh port %s", port->id);
                    cup = new clap::UIMeshPort(cp);
                    break;

                case meta::R_STREAM:
                    lsp_trace("creating stream port %s", port->id);
                    cup = new clap::UIStreamPort(cp);
                    break;

                case meta::R_FBUFFER:
                    lsp_trace("creating fbuffer port %s", port->id);
                    cup = new clap::UIFrameBufferPort(cp);
                    break;

                case meta::R_OSC:
                    lsp_trace("creating osc port %s", port->id);
                    if (meta::is_out_port(port))
                        cup     = new clap::UIOscPortIn(port, cp);
                    else
                        cup     = new clap::UIOscPortOut(port, cp);
                    break;

                case meta::R_PATH:
                    lsp_trace("creating path port %s", port->id);
                    cup = new clap::UIPathPort(cp);
                    break;

                case meta::R_CONTROL:
                case meta::R_METER:
                case meta::R_BYPASS:
                    lsp_trace("creating regular port %s", port->id);
                    // VST specifies only INPUT parameters, output should be read in different way
                    if (meta::is_out_port(port))
                        cup     = new clap::UIMeterPort(cp);
                    else
                        cup     = new clap::UIParameterPort(static_cast<clap::ParameterPort *>(cp), &bRequestProcess);
                    break;

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES], param_name[MAX_PARAM_ID_BYTES];
                    lsp_trace("creating port group %s", port->id);
                    UIPortGroup *upg = new clap::UIPortGroup(static_cast<clap::PortGroup *>(cp), &bRequestProcess);

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
                            cp    = pWrapper->find_by_id(param_name);
                            lsp_trace("find_by_id %s -> %p", param_name, cp);
                            if (cp != NULL)
                                create_port(cp->metadata(), postfix_buf);
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            // Add port to the list of UI ports
            if (cup != NULL)
                vPorts.add(cup);

            return cup;
        }

        void UIWrapper::tranfet_ui_to_dsp()
        {
            if (bRequestProcess)
            {
                if ((pExt != NULL) && (pExt->host != NULL))
                    pExt->host->request_process(pExt->host);
                bRequestProcess = false;
            }
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
                clap::UIPort *cup   = static_cast<clap::UIPort *>(vPorts.uget(i));
                do {
                    if (cup->sync())
                        cup->notify_all(ui::PORT_NONE);
                } while (cup->sync_again());
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

        const meta::package_t *UIWrapper::package() const
        {
            return pWrapper->package();
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

        float UIWrapper::ui_scaling_factor(float scaling)
        {
            return (fScaling > 0.0f) ? fScaling : scaling;
        }

        bool UIWrapper::accept_window_size(tk::Window *wnd, size_t width, size_t height)
        {
            if ((pExt == NULL) || (wnd != wWindow))
                return IWrapper::accept_window_size(wnd, width, height);

            return pExt->gui->request_resize(pExt->host, width, height);
        }

        meta::plugin_format_t UIWrapper::plugin_format() const
        {
            return meta::PLUGIN_CLAP;
        }

        status_t UIWrapper::slot_ui_resize(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = p%", sender, ptr, data);

            UIWrapper *this_    = static_cast<UIWrapper *>(ptr);
            tk::Window *wnd     = this_->window();
            if ((wnd == NULL) || (!wnd->visibility()->get()))
                return STATUS_OK;

            ws::rectangle_t rr;
            if (wnd->get_screen_rectangle(&rr) != STATUS_OK)
                return STATUS_OK;

            this_->pExt->gui->request_resize(this_->pExt->host, rr.nWidth, rr.nHeight);

            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_show(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = p%", sender, ptr, data);
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_realized(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = p%", sender, ptr, data);

            UIWrapper *this_    = static_cast<UIWrapper *>(ptr);
            tk::Window *wnd     = this_->window();

            ws::rectangle_t rr;
            if (wnd->get_screen_rectangle(&rr) != STATUS_OK)
                return STATUS_OK;
            this_->pExt->gui->request_resize(this_->pExt->host, rr.nWidth, rr.nHeight);

            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_close(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = p%", sender, ptr, data);
            UIWrapper *this_ = static_cast<UIWrapper *>(ptr);
            if (this_->pExt != NULL)
                this_->pExt->gui->closed(this_->pExt->host, false);
            return STATUS_OK;
        }

        bool UIWrapper::set_scale(double scale)
        {
            // Scale is in %
            fScaling    = scale * 100.0f;

            // Trigger scaling factor to become updated
            if (!sMutex.lock())
                return false;
            lsp_finally { sMutex.unlock(); };

            ctl::PluginWindow *wnd = ctl::ctl_cast<ctl::PluginWindow>(pWindow);
            if (wnd != NULL)
                wnd->host_scaling_changed();

            return true;
        }

        bool UIWrapper::get_size(uint32_t *width, uint32_t *height)
        {
            tk::Window *wnd     = window();
            if (wnd == NULL)
                return false;

            // Lock the access to the main loop
            if (!sMutex.lock())
                return false;
            lsp_finally { sMutex.unlock(); };

            // Obtain the window parameters
            if (wnd->visibility()->get())
            {
                ws::rectangle_t rr;
                rr.nWidth       = 0;
                rr.nHeight      = 0;

                if (wnd->get_screen_rectangle(&rr) != STATUS_OK)
                    return false;

                // Return result
                if (width != NULL)
                    *width  = rr.nWidth;
                if (height != NULL)
                    *height = rr.nHeight;
            }
            else
            {
                ws::size_limit_t sr;
                wnd->get_size_limits(&sr);

                // Return result
                if (width != NULL)
                    *width  = lsp_min(sr.nMinWidth, 32);
                if (height != NULL)
                    *height = lsp_min(sr.nMinHeight, 32);
            }

            return true;
        }

        bool UIWrapper::can_resize()
        {
            tk::Window *wnd     = window();
            if (wnd == NULL)
                return false;

            if (!sMutex.lock())
                return false;
            lsp_finally { sMutex.unlock(); };

            return wnd->border_style()->get() != ws::BS_DIALOG;
        }

        bool UIWrapper::get_resize_hints(clap_gui_resize_hints_t *hints)
        {
            tk::Window *wnd     = window();
            if (wnd == NULL)
                return false;

            if (!sMutex.lock())
                return false;
            lsp_finally { sMutex.unlock(); };

            // Obtain window size constraints
            ws::size_limit_t sr;
            wnd->get_padded_size_limits(&sr);

            hints->can_resize_horizontally = (sr.nMaxWidth < 0) || (sr.nMaxWidth  > sr.nMinWidth);
            hints->can_resize_vertically    = (sr.nMaxHeight< 0) || (sr.nMaxHeight > sr.nMinHeight);

            hints->preserve_aspect_ratio    = false;
            hints->aspect_ratio_width       = 1;
            hints->aspect_ratio_height      = 1;

            return true;
        }

        bool UIWrapper::adjust_size(uint32_t *width, uint32_t *height)
        {
            tk::Window *wnd     = window();
            if (wnd == NULL)
                return false;

            if (!sMutex.lock())
                return false;
            lsp_finally { sMutex.unlock(); };

            ws::size_limit_t sr;
            wnd->get_padded_size_limits(&sr);

            ws::rectangle_t r;
            r.nLeft     = 0;
            r.nTop      = 0;
            r.nWidth    = *width;
            r.nHeight   = *height;

            tk::SizeConstraints::apply(&r, &sr);

            *width      = r.nWidth;
            *height     = r.nHeight;

            return true;
        }

        bool UIWrapper::set_size(uint32_t width, uint32_t height)
        {
            tk::Window *wnd     = window();
            if (wnd == NULL)
                return false;

            if (!sMutex.lock())
                return false;
            lsp_finally { sMutex.unlock(); };

            wnd->resize_window(width, height);

            return true;
        }

        void *UIWrapper::to_native_handle(const clap_window_t *window)
        {
        #if defined(PLATFORM_WINDOWS)
            return static_cast<void *>(window->win32);
        #elif defined(PLATFORM_MACOSX)
            return static_cast<void *>(window->cocoa);
        #else
            return reinterpret_cast<void *>(window->x11);
        #endif
        }

        bool UIWrapper::set_parent(const clap_window_t *window)
        {
            pParent     = to_native_handle(window);
            return initialize_ui();
        }

        bool UIWrapper::set_transient(const clap_window_t *window)
        {
            if (pDisplay == NULL)
                return false;

            if (pTransientFor != NULL)
                delete pTransientFor;

            void *handle    = to_native_handle(window);
            pTransientFor   = (handle != NULL) ? pDisplay->display()->wrap_window(handle) : NULL;

            return true;
        }

        void UIWrapper::suggest_title(const char *title)
        {
            tk::Window *wnd     = window();
            if (wnd == NULL)
                return;
            if (!sMutex.lock())
                return;
            lsp_finally { sMutex.unlock(); };

            // Update the window title
            wnd->title()->set_raw(title);
        }

        bool UIWrapper::initialize_ui()
        {
            if (bUIInitialized)
                return true;

            lsp_trace("Initializing UI contents");

            status_t res = STATUS_OK;
            const meta::plugin_t *meta = pUI->metadata();
            if (pUI->metadata() == NULL)
                return false;

            if (meta->ui_resource != NULL)
            {
                if ((res = build_ui(meta->ui_resource, pParent)) != STATUS_OK)
                {
                    lsp_error("Error building UI for resource %s: code=%d", meta->ui_resource, int(res));
                    return false;
                }
            }

            // Bind resize slot
            lsp_trace("Binding slots");
            tk::Window *wnd  = window();
            if (wnd != NULL)
            {
                wnd->slots()->bind(tk::SLOT_RESIZE, slot_ui_resize, this);
                wnd->slots()->bind(tk::SLOT_SHOW, slot_ui_show, this);
                wnd->slots()->bind(tk::SLOT_REALIZED, slot_ui_realized, this);
                wnd->slots()->bind(tk::SLOT_CLOSE, slot_ui_close, this);
            }

            // Call the post-initialization routine
            lsp_trace("Doing post-init");
            res = pUI->post_init();
            bUIInitialized = (res == STATUS_OK);
            lsp_trace("bUIInitialized = %d, res=%d", int(bUIInitialized), int(res));

            return bUIInitialized;
        }

        bool UIWrapper::ui_active() const
        {
            return bUIActive;
        }

        void UIWrapper::main_iteration()
        {
            if (!bUIActive)
                return;

            // Perform main data transfers and state sync
            tranfet_ui_to_dsp();
            transfer_dsp_to_ui();
            IWrapper::main_iteration();
        }

        bool UIWrapper::show()
        {
            // Lazy initialize the UI ONLY after whe have the parent/transient settings
            if (!initialize_ui())
                return false;

            tk::Window *wnd  = window();
            if (wnd == NULL)
                return false;

            // Show the window and start event loop
            wnd->show(pTransientFor);
            bool res    = start_event_loop();
            if (!res)
            {
                wnd->hide();
                return res;
            }

            // Update UI status
            bUIActive       = true;
            if (pWrapper != NULL)
                pWrapper->ui_visibility_changed();

            return true;
        }

        bool UIWrapper::hide()
        {
            // Update UI status
            bUIActive       = false;
            if (pWrapper != NULL)
                pWrapper->ui_visibility_changed();

            // Stop the event loop
            stop_event_loop();

            // Do the sync barrier
            sMutex.lock();
            lsp_finally { sMutex.unlock(); };

            // Hide the window
            tk::Window *wnd = window();
            if (wnd != NULL)
                wnd->hide();

            return true;
        }

        status_t UIWrapper::slot_display_idle(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *self = static_cast<UIWrapper *>(ptr);
            if (self != NULL)
                self->main_iteration();

            return STATUS_OK;
        }

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_UI_WRAPPER_H_ */
