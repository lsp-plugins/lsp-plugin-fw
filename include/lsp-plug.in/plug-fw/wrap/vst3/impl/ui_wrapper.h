/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 янв. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/controller.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/debug.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {

        UIWrapper::UIWrapper(vst3::Controller *controller, ui::Module *ui, resource::ILoader *loader):
            ui::IWrapper(ui, loader)
        {
            lsp_trace("this=%p", this);

            atomic_store(&nRefCounter, 1);
            pController         = safe_acquire(controller);
            pPlugFrame          = NULL;
            fScalingFactor      = -1.0f;
            atomic_store(&nPlayPositionReq, 0);
            nPlayPositionResp   = 0;

        #ifdef VST_USE_RUNLOOP_IFACE
            pRunLoop            = NULL;
            pTimer              = safe_acquire(new vst3::PlatformTimer(this));
        #endif /* VST_USE_RUNLOOP_IFACE */
        }

        UIWrapper::~UIWrapper()
        {
            lsp_trace("this=%p", this);

            // Remove self from synchronization list of UI wrapper
            if (pController != NULL)
                pController->detach_ui_wrapper(this);

            // Destroy
            do_destroy();

            // Release factory
            safe_release(pPlugFrame);
            safe_release(pController);
        }

        vst3::UIPort *UIWrapper::create_port(const meta::port_t *port, const char *postfix)
        {
            // Find the matching port for the backend
            vst3::UIPort *vup   = NULL;
            vst3::CtlPort *p    = pController->port_by_id(port->id);
            if (p == NULL)
            {
                lsp_warn("Could not find controller port id=%s", port->id);
                return NULL;
            }

            switch (port->role)
            {
                case meta::R_AUDIO_IN:
                case meta::R_AUDIO_OUT:
                    // Stub port
                    lsp_trace("creating stub audio port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_MIDI_IN:
                case meta::R_MIDI_OUT:
                    // Stub port
                    lsp_trace("creating stub midi port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_AUDIO_SEND:
                    lsp_trace("creating audio send port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_AUDIO_RETURN:
                    lsp_trace("creating audio return port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_MESH:
                    lsp_trace("creating mesh port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_STREAM:
                    lsp_trace("creating stream port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_FBUFFER:
                    lsp_trace("creating fbuffer port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                    break;

                case meta::R_PATH:
                    lsp_trace("creating path port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_STRING:
                case meta::R_SEND_NAME:
                case meta::R_RETURN_NAME:
                    lsp_trace("creating string port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    lsp_trace("creating parameter port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;
                }

                case meta::R_METER:
                {
                    lsp_trace("creating meter port %s", port->id);
                    vup     = new vst3::UIPort(p);
                    break;
                }

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    char port_id[MAX_PARAM_ID_BYTES];

                    CtlPortGroup *cpg = static_cast<CtlPortGroup *>(p);

                    lsp_trace("creating port group %s of %d rows", port->id, int(cpg->rows()));
                    vst3::UIPort *upg = new vst3::UIPort(p);

                    // Add immediately port group to list
                    vPorts.add(upg);
                    vSync.add(upg);

                    // Add nested ports
                    for (size_t row=0; row<cpg->rows(); ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Create all nested ports in port group
                        for (const meta::port_t *cm = port->members; cm->id != NULL; ++cm)
                        {
                            strcpy(port_id, cm->id);
                            strcat(port_id, postfix_buf);

                            p       = pController->port_by_id(port_id);
                            if (p != NULL)
                                create_port(p->metadata(), postfix_buf);
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            // Add port to the list of UI ports
            if (vup != NULL)
            {
                vPorts.add(vup);
                vSync.add(vup);
            }

            return vup;
        }

        status_t UIWrapper::init(void *root_widget)
        {
            lsp_trace("this=%p", this);

            status_t res;

            // Get plugin metadata
            const meta::plugin_t *meta  = pUI->metadata();
            if (meta == NULL)
            {
                lsp_warn("No plugin metadata found");
                return STATUS_BAD_STATE;
            }

            // Perform all port bindings
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(port, NULL);

            // Initialize wrapper
            if ((res = ui::IWrapper::init(root_widget)) != STATUS_OK)
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

            lsp_trace("Initializing UI contents");
            if (meta->ui_resource != NULL)
            {
                if ((res = build_ui(meta->ui_resource, NULL)) != STATUS_OK)
                {
                    lsp_error("Error building UI for resource %s: code=%d", meta->ui_resource, int(res));
                    return res;
                }
            }

            // Bind different slots
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
            if ((res = pUI->post_init()) != STATUS_OK)
                return res;

            for (lltl::iterator<ui::IPort> it = vPorts.values(); it; ++it)
            {
                ui::IPort *p = it.get();
                if (p != NULL)
                    p->notify_all(ui::PORT_NONE);
            }

            return STATUS_OK;
        }

        void UIWrapper::do_destroy()
        {
            lsp_trace("this=%p", this);

            // Cleanup synchronization ports
            vSync.flush();

            // Destroy plugin UI
            if (pUI != NULL)
            {
                delete pUI;
                pUI         = NULL;
            }

            ui::IWrapper::destroy();

            // Destroy display object
            if (pDisplay != NULL)
            {
                pDisplay->destroy();
                pDisplay    = NULL;
            }
        }

        void UIWrapper::destroy()
        {
            do_destroy();
            IWrapper::destroy();
        }

        core::KVTStorage *UIWrapper::kvt_lock()
        {
            return (pController->kvt_mutex().lock()) ? pController->kvt_storage() : NULL;
        }

        core::KVTStorage *UIWrapper::kvt_trylock()
        {
            return (pController->kvt_mutex().try_lock()) ? pController->kvt_storage() : NULL;
        }

        bool UIWrapper::kvt_release()
        {
            return pController->kvt_mutex().unlock();
        }

        void UIWrapper::dump_state_request()
        {
            pController->dump_state_request();
        }

        const meta::package_t *UIWrapper::package() const
        {
            return pController->package();
        }

        meta::plugin_format_t UIWrapper::plugin_format() const
        {
            return meta::PLUGIN_VST3;
        }

        status_t UIWrapper::play_file(const char *file, wsize_t position, bool release)
        {
            return pController->play_file(file, position, release);
        }

        float UIWrapper::ui_scaling_factor(float scaling)
        {
            return (fScalingFactor > 0.0f) ? fScalingFactor : scaling;
        }

        void UIWrapper::sync_kvt_state(core::KVTStorage *kvt)
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
                kvt->commit_all(core::KVT_RX);  // Just clear all RX queue for non-debug version
            #endif /* LSP_DEBUG */
        }

        void UIWrapper::main_iteration()
        {
            sync_with_controller();
            sync_with_dsp();

            // Synchronize play position
            uatomic_t play_req = atomic_load(&nPlayPositionReq);
            if (play_req != nPlayPositionResp)
            {
                lltl::parray<ui::IPlayListener> listeners;
                listeners.add(vPlayListeners);
                for (size_t i=0; i<vPlayListeners.size(); ++i)
                {
                    ui::IPlayListener *listener = vPlayListeners.uget(i);
                    if (listener != NULL)
                        listener->play_position_update(nPlayPosition, nPlayLength);
                }

                // Commit response ID
                nPlayPositionResp   = play_req;
            }

            // Transmit KVT state
            core::KVTStorage *kvt = kvt_trylock();
            if (kvt != NULL)
            {
                sync_kvt_state(kvt);
                kvt->gc();
                kvt_release();
            }

            // Call for parent
            ui::IWrapper::main_iteration();
        }

        void UIWrapper::sync_with_controller()
        {
            for (lltl::iterator<UIPort> it = vSync.values(); it; ++it)
            {
                UIPort *p = it.get();
                if (p != NULL)
                    p->sync();
            }
        }

        void UIWrapper::sync_with_dsp()
        {
        }

        void UIWrapper::commit_position(const plug::position_t *pos)
        {
            position_updated(pos);
        }

        void UIWrapper::set_play_position(wssize_t position, wssize_t length)
        {
            // Update position and increment the change counter
            nPlayPosition       = position;
            nPlayLength         = length;
            atomic_add(&nPlayPositionReq, 1);
        }

        Steinberg::tresult UIWrapper::show_about_box()
        {
            lsp_trace("this=%p", this);

            ctl::PluginWindow *wnd = ctl::ctl_cast<ctl::PluginWindow>(pWindow);
            if (wnd == NULL)
                return Steinberg::kResultFalse;

            status_t res = wnd->show_about_window();
            return (res == STATUS_OK) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        Steinberg::tresult UIWrapper::show_help()
        {
            lsp_trace("this=%p", this);

            ctl::PluginWindow *wnd = ctl::ctl_cast<ctl::PluginWindow>(pWindow);
            if (wnd == NULL)
                return Steinberg::kResultFalse;

            status_t res = wnd->show_plugin_manual();
            return (res == STATUS_OK) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::queryInterface(const Steinberg::TUID _iid, void **obj)
        {
            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                return cast_interface<Steinberg::FUnknown>(static_cast<Steinberg::IDependent *>(this), obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IDependent::iid))
                return cast_interface<Steinberg::IDependent>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPlugView::iid))
                return cast_interface<Steinberg::IPlugView>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPlugViewContentScaleSupport::iid))
                return cast_interface<Steinberg::IPlugViewContentScaleSupport>(this, obj);

            return no_interface(obj);
        }

        Steinberg::uint32 PLUGIN_API UIWrapper::addRef()
        {
            return atomic_add(&nRefCounter, 1) + 1;
        }

        Steinberg::uint32 PLUGIN_API UIWrapper::release()
        {
            atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        void PLUGIN_API UIWrapper::update(FUnknown *changedUnknown, Steinberg::int32 message)
        {
            lsp_trace("this=%p, changedUnknown=%p, message=%d", this, changedUnknown, int(message));
        }

        Steinberg::tresult PLUGIN_API UIWrapper::isPlatformTypeSupported(Steinberg::FIDString type)
        {
            bool supported = false;
#if defined(PLATFORM_WINDOWS)
            supported = (strcmp(type, Steinberg::kPlatformTypeHWND) == 0);
#elif defined(PLATFORM_MACOSX)
#else
            supported = (strcmp(type, Steinberg::kPlatformTypeX11EmbedWindowID) == 0);
#endif
            return (supported) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::attached(void *parent, Steinberg::FIDString type)
        {
            lsp_trace("this=%p, parent=%p, type=%s", this, parent, type);

            if (isPlatformTypeSupported(type) != Steinberg::kResultTrue)
                return Steinberg::kResultFalse;

        #ifdef VST_USE_RUNLOOP_IFACE
            // Register the timer for event loop
            lsp_trace("this=%p, pRunLoop=%p, pTimer=%p", this, pRunLoop, pTimer);
            if ((pRunLoop != NULL) && (pTimer != NULL))
                pRunLoop->registerTimer(pTimer, 1000 / UI_FRAMES_PER_SECOND);
        #endif /* VST_USE_RUNLOOP_IFACE */

            // Show the window
            if (wWindow == NULL)
            {
                lsp_trace("wWindow == NULL");
                return Steinberg::kResultFalse;
            }

            wWindow->native()->set_parent(parent);
            wWindow->position()->set(0, 0);
            wWindow->show();

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::removed()
        {
            lsp_trace("this=%p", this);

            // Hide the window
            if (wWindow == NULL)
                return Steinberg::kResultFalse;

            wWindow->hide();
            wWindow->native()->set_parent(NULL);

        #ifdef VST_USE_RUNLOOP_IFACE
            // Unregister the timer for event loop
            if ((pRunLoop != NULL) && (pTimer != NULL))
                pRunLoop->unregisterTimer(pTimer);
        #endif /* VST_USE_RUNLOOP_IFACE */

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::onWheel(float distance)
        {
            // No, please!
            return Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::onKeyDown(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers)
        {
            // No, please!
            return Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::onKeyUp(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers)
        {
            // No, please!
            return Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getSize(Steinberg::ViewRect *size)
        {
            lsp_trace("this=%p, size=%p", this, size);

            if (wWindow == NULL)
            {
                lsp_trace("wnd == NULL");
                return Steinberg::kResultFalse;
            }

            // Obtain the window parameters
            if (wWindow->visibility()->get())
            {
                lsp_trace("window is visible");

                ws::rectangle_t rr;
                rr.nLeft        = 0;
                rr.nTop         = 0;
                rr.nWidth       = 0;
                rr.nHeight      = 0;

                wWindow->get_padded_rectangle(&rr);

                // Return result
                size->left      = rr.nLeft;
                size->top       = rr.nTop;
                size->right     = rr.nLeft + rr.nWidth;
                size->bottom    = rr.nTop  + rr.nHeight;
            }
            else
            {
                lsp_trace("window is not visible");

                ws::size_limit_t sr;
                wWindow->get_size_limits(&sr);

                size->left      = 0;
                size->top       = 0;
                size->right     = lsp_min(sr.nMinWidth, 32);
                size->bottom    = lsp_min(sr.nMinHeight, 32);
            }

            lsp_trace("this=%p, size={left=%d, top=%d, right=%d, bottom=%d}",
                this, int(size->left), int(size->top), int(size->right), int(size->bottom));

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::onSize(Steinberg::ViewRect *newSize)
        {
            lsp_trace("this=%p, newSize={left=%d, top=%d, right=%d, bottom=%d}",
                this, int(newSize->left), int(newSize->top), int(newSize->right), int(newSize->bottom));

            if (wWindow == NULL)
            {
                lsp_trace("wnd == NULL");
                return Steinberg::kResultFalse;
            }

            Steinberg::tresult res = checkSizeConstraint(newSize);
            if (res != Steinberg::kResultOk)
                return res;

            lsp_trace("RESIZE TO this=%p, newSize={left=%d, top=%d, right=%d, bottom=%d}",
                this, int(newSize->left), int(newSize->top), int(newSize->right), int(newSize->bottom));
            wWindow->position()->set(newSize->left, newSize->top);
            wWindow->size()->set(newSize->right - newSize->left, newSize->bottom - newSize->top);

            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::onFocus(Steinberg::TBool state)
        {
            // No, please
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setFrame(Steinberg::IPlugFrame *frame)
        {
            lsp_trace("this=%p, frame=%p", this, frame);

            safe_release(pPlugFrame);
            pPlugFrame  = safe_acquire(frame);

        #ifdef VST_USE_RUNLOOP_IFACE
            // Acquire new pointer to the run loop
            safe_release(pRunLoop);
            pRunLoop = safe_query_iface<Steinberg::Linux::IRunLoop>(frame);
            if (pRunLoop == NULL)
                pRunLoop    = pController->acquire_run_loop();

           lsp_trace("RUN LOOP object=%p", pRunLoop);
        #endif /* VST_USE_RUNLOOP_IFACE */

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::canResize()
        {
            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::checkSizeConstraint(Steinberg::ViewRect *rect)
        {
            lsp_trace("this=%p, rect={left=%d, top=%d, right=%d, bottom=%d}",
                this, int(rect->left), int(rect->top), int(rect->right), int(rect->bottom));

            if (wWindow == NULL)
                return Steinberg::kResultFalse;

            ws::rectangle_t sr, dr;
            sr.nLeft    = rect->left;
            sr.nTop     = rect->top;
            sr.nWidth   = rect->right  - rect->left;
            sr.nHeight  = rect->bottom - rect->top;

            ws::size_limit_t sl;
            wWindow->get_padded_size_limits(&sl);

            lsp_trace("this=%p, width={min=%d, max=%d, pre=%d}, height={min=%d, max=%d, pre=%d}",
                this,
                int(sl.nMinWidth), int(sl.nMaxWidth), int(sl.nPreWidth),
                int(sl.nMinHeight), int(sl.nMaxHeight), int(sl.nPreHeight));

            tk::SizeConstraints::apply(&dr, &sr, &sl);

            lsp_trace("this=%p, constrained={left=%d, top=%d, width=%d, height=%d}",
                this, int(dr.nLeft), int(dr.nTop), int(dr.nWidth), int(dr.nHeight));

            // Update the rect if it was constrained
            if ((dr.nWidth != sr.nWidth) || (dr.nHeight != sr.nHeight))
            {
                lsp_trace("this=%p, applied={left=%d, top=%d, right=%d, bottom=%d}",
                    this, int(rect->left), int(rect->top), int(rect->right), int(rect->bottom));

                rect->right  = rect->left + dr.nWidth;
                rect->bottom = rect->top  + dr.nHeight;
            }

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor)
        {
            lsp_trace("this=%p, factor=%f", this, factor);
            fScalingFactor = factor * 100.0f;

            ctl::PluginWindow *wnd = ctl::ctl_cast<ctl::PluginWindow>(pWindow);
            if (wnd != NULL)
                wnd->host_scaling_changed();

            return Steinberg::kResultOk;
        }

        void UIWrapper::sync_ui()
        {
            if (pDisplay != NULL)
                pDisplay->main_iteration();
        }

        void UIWrapper::query_resize(const ws::rectangle_t *r)
        {
            lsp_trace("this=%p, width=%d, height=%d", this, int(r->nWidth), int(r->nHeight));

            if (pPlugFrame == NULL)
                return;

            Steinberg::ViewRect newSize;
            newSize.left        = 0;
            newSize.top         = 0;
            newSize.right       = r->nWidth;
            newSize.bottom      = r->nHeight;

            pPlugFrame->resizeView(this, &newSize);
        }

        status_t UIWrapper::slot_ui_resize(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);

            UIWrapper *self     = static_cast<UIWrapper *>(ptr);
            tk::Window *wnd     = self->window();
            if ((wnd == NULL) || (!wnd->visibility()->get()))
            {
                lsp_trace("(wnd == NULL) || (!wnd->visibility()->get())");
                return STATUS_OK;
            }

            ws::rectangle_t rr;
            if (wnd->get_screen_rectangle(&rr) != STATUS_OK)
            {
                lsp_trace("wnd->get_screen_rectangle(&rr) != STATUS_OK");
                return STATUS_OK;
            }

            self->query_resize(&rr);

            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_show(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_realized(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_close(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            return STATUS_OK;
        }

        status_t UIWrapper::slot_display_idle(tk::Widget *sender, void *ptr, void *data)
        {
//            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            UIWrapper *self = static_cast<UIWrapper *>(ptr);
            if (self != NULL)
                self->main_iteration();

            return STATUS_OK;
        }

        const core::ShmState *UIWrapper::shm_state()
        {
            return pController->shm_state();
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_UI_WRAPPER_H_ */
