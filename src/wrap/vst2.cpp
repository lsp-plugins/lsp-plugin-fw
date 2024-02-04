/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 дек. 2021 г.
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

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/impl/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/impl/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/func.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/ws/keycodes.h>

#ifdef USE_LIBX11
    #include <lsp-plug.in/ws/x11/decode.h>
#endif /* USE_LIBX11 */

#define VST2_LOG_FILE   "lsp-vst2-aeffect.log"

namespace lsp
{
    namespace vst2
    {
        static const VstInt32 VST2_MAGIC_stCA        = CCONST('s', 't', 'C', 'A');
        static const VstInt32 VST2_MAGIC_stCa        = CCONST('s', 't', 'C', 'a');
        static const VstInt32 VST2_MAGIC_FUID        = CCONST('F', 'U', 'I', 'D');

        typedef struct key_code_t
        {
            uint8_t     vst;
            ws::code_t  ws;
        } key_code_t;

        static key_code_t key_table[] =
        {
            { VKEY_BACK,      ws::WSK_BACKSPACE         },
            { VKEY_TAB,       ws::WSK_TAB               },
            { VKEY_RETURN,    ws::WSK_RETURN            },
            { VKEY_PAUSE,     ws::WSK_PAUSE             },
            { VKEY_ESCAPE,    ws::WSK_ESCAPE            },
            { VKEY_SPACE,     ' '                       },
            { VKEY_END,       ws::WSK_END               },
            { VKEY_HOME,      ws::WSK_HOME              },
            { VKEY_LEFT,      ws::WSK_LEFT              },
            { VKEY_UP,        ws::WSK_UP                },
            { VKEY_RIGHT,     ws::WSK_RIGHT             },
            { VKEY_DOWN,      ws::WSK_DOWN              },
            { VKEY_PAGEUP,    ws::WSK_PAGE_UP           },
            { VKEY_PAGEDOWN,  ws::WSK_PAGE_DOWN         },
            { VKEY_PRINT,     ws::WSK_PRINT             },
            { VKEY_ENTER,     ws::WSK_KEYPAD_ENTER      },
            { VKEY_HELP,      ws::WSK_HELP              },
            { VKEY_DELETE,    ws::WSK_DELETE            },
            { VKEY_NUMPAD0,   ws::WSK_KEYPAD_0          },
            { VKEY_NUMPAD1,   ws::WSK_KEYPAD_1          },
            { VKEY_NUMPAD2,   ws::WSK_KEYPAD_2          },
            { VKEY_NUMPAD3,   ws::WSK_KEYPAD_3          },
            { VKEY_NUMPAD4,   ws::WSK_KEYPAD_4          },
            { VKEY_NUMPAD5,   ws::WSK_KEYPAD_5          },
            { VKEY_NUMPAD6,   ws::WSK_KEYPAD_6          },
            { VKEY_NUMPAD7,   ws::WSK_KEYPAD_7          },
            { VKEY_NUMPAD8,   ws::WSK_KEYPAD_8          },
            { VKEY_NUMPAD9,   ws::WSK_KEYPAD_9          },
            { VKEY_MULTIPLY,  ws::WSK_KEYPAD_MULTIPLY   },
            { VKEY_ADD,       ws::WSK_KEYPAD_ADD        },
            { VKEY_SUBTRACT,  ws::WSK_KEYPAD_SUBTRACT   },
            { VKEY_DECIMAL,   ws::WSK_KEYPAD_DECIMAL    },
            { VKEY_DIVIDE,    ws::WSK_KEYPAD_DIVIDE     },
            { VKEY_F1,        ws::WSK_F1                },
            { VKEY_F2,        ws::WSK_F2                },
            { VKEY_F3,        ws::WSK_F3                },
            { VKEY_F4,        ws::WSK_F4                },
            { VKEY_F5,        ws::WSK_F5                },
            { VKEY_F6,        ws::WSK_F6                },
            { VKEY_F7,        ws::WSK_F7                },
            { VKEY_F8,        ws::WSK_F8                },
            { VKEY_F9,        ws::WSK_F9                },
            { VKEY_F10,       ws::WSK_F10               },
            { VKEY_F11,       ws::WSK_F11               },
            { VKEY_F12,       ws::WSK_F12               },
            { VKEY_NUMLOCK,   ws::WSK_NUM_LOCK          },
            { VKEY_SCROLL,    ws::WSK_SCROLL_LOCK       },
            { VKEY_SHIFT,     ws::WSK_SHIFT_L           },
            { VKEY_CONTROL,   ws::WSK_CONTROL_L         },
            { VKEY_ALT,       ws::WSK_ALT_L             },
            { VKEY_EQUALS,    ws::WSK_KEYPAD_EQUAL      },
        };

        int process_key_event(vst2::UIWrapper *w, VstInt32 opcode, VstInt32 index, VstIntPtr value)
        {
            int v = 0;
            tk::Window *wnd = w->window();
            if (wnd == NULL)
                return v;

            // Generate user event
            ws::event_t ue;
            ws::init_event(&ue);
            ue.nType        = (opcode == effEditKeyDown) ? ws::UIE_KEY_DOWN : ws::UIE_KEY_UP;

            // Process value first
            if (value > 0)
            {
                // Translate the code
                ws::code_t code = ws::WSK_UNKNOWN;
                for (size_t i=0, n=sizeof(key_table)/sizeof(key_code_t); i<n; ++i)
                {
                    if (value == key_table[i].vst)
                    {
                        code = key_table[i].ws;
                        break;
                    }
                }

                // Post-process the code
                if (code != ws::WSK_UNKNOWN)
                {
                    ue.nCode    = code;
                    ue.nState   = w->key_state();
                    wnd->handle_event(&ue);
                    v = 1;
                }

                // Update state of the keyboard
                bool down = (ue.nType == ws::UIE_KEY_DOWN);
                if ((code == ws::WSK_SHIFT_L) || (code == ws::WSK_SHIFT_R))
                    w->set_key_state(lsp_setflag(w->key_state(), ws::MCF_SHIFT, down));
                else if ((code == ws::WSK_ALT_L) || (code == ws::WSK_ALT_R))
                    w->set_key_state(lsp_setflag(w->key_state(), ws::MCF_ALT, down));
                else if ((code == ws::WSK_CONTROL_L) || (code == ws::WSK_CONTROL_R))
                    w->set_key_state(lsp_setflag(w->key_state(), ws::MCF_CONTROL, down));
            }

            // Process index second
            if (index > 0)
            {
            #ifdef USE_LIBX11
                ws::code_t code = ws::x11::decode_keycode(index);
            #else
                ws::code_t code = index;
            #endif /* USE_LIBX11 */

                ue.nCode    = code;
                ue.nState   = w->key_state();
                wnd->handle_event(&ue);
                v = 1;
            }

            return v;
        }

        void finalize(AEffect *e)
        {
            lsp_trace("effect=%p", e);
            if (e == NULL)
                return;

            // Get VST object
            vst2::Wrapper *w     = reinterpret_cast<vst2::Wrapper *>(e->object);
            if (w != NULL)
            {
                w->destroy();
                delete w;

                e->object           = NULL;
            }

            // Delete audio effect
            delete e;
        }

        void get_parameter_properties(const meta::port_t *m, VstParameterProperties *p)
        {
            lsp_trace("parameter id=%s, name=%s", m->id, m->name);

            float min = 0.0f, max = 1.0f, step = 0.001f;
            get_port_parameters(m, &min, &max, &step);

            vst_strncpy(p->label, m->name, kVstMaxLabelLen);
            p->flags                    = 0;
            p->minInteger               = min;
            p->maxInteger               = max;
            p->stepInteger              = step;
            p->largeStepInteger         = step;

            p->stepFloat                = (max != min) ? step / (max - min) : 0.0f;
            p->smallStepFloat           = p->stepFloat;
            p->largeStepFloat           = p->stepFloat;

            vst_strncpy(p->shortLabel, m->id, kVstMaxShortLabelLen);

            if (m->unit == meta::U_BOOL)
                p->flags                    = kVstParameterIsSwitch;

            p->stepFloat                = (max != min) ? step / (max - min) : 0.0f;
            p->smallStepFloat           = p->stepFloat;
            p->largeStepFloat           = p->stepFloat;

    //        // This code may crash the hosts that use VesTige, so for capability issues it is commented
    //        p->displayIndex             = 0;
    //        p->category                 = 0;
    //        p->numParametersInCategory  = 0;
    //        p->reserved                 = 0;
    //        p->categoryLabel[0]         = '\0';

    // Not all hosts are ready to properly handle these features
    //        if (m->unit == U_BOOL)
    //            p->flags                    = kVstParameterIsSwitch | kVstParameterUsesIntegerMinMax | kVstParameterUsesIntStep;
    //        else if (m->unit == U_ENUM)
    //            p->flags                    = kVstParameterUsesIntegerMinMax | kVstParameterUsesIntStep;
    //        else if (m->unit == U_SAMPLES)
    //            p->flags                    = kVstParameterUsesIntegerMinMax | kVstParameterUsesIntStep;
    //        else
    //        {
    //            if (m->flags & F_INT)
    //                p->flags                    = kVstParameterUsesIntegerMinMax | kVstParameterUsesIntStep;
    //            else
    //                p->flags                    = kVstParameterUsesFloatStep;
    //        }
    //
    //        if (p->flags & kVstParameterUsesIntStep)
    //        {
    //            p->stepFloat                = step;
    //            p->smallStepFloat           = step;
    //            p->largeStepFloat           = step;
    //        }
    //        else
    //        {
    //            p->stepFloat                = (max != min) ? step / (max - min) : 0.0f;
    //            p->smallStepFloat           = p->stepFloat;
    //            p->largeStepFloat           = p->stepFloat;
    //        }
        }

    #ifdef LSP_DEBUG
        const char *decode_opcode(VstInt32 opcode)
        {
            const char *r = NULL;

        #define C(code) case code: r = #code; break;

            switch (opcode)
            {
                C(effGetVstVersion)
                C(effClose)
                C(effGetVendorString)
                C(effGetProductString)
                C(effGetParamName)
                C(effGetParamLabel)
                C(effGetParamDisplay)
                C(effCanBeAutomated)
                C(effGetParameterProperties)
                C(effSetSampleRate)
                C(effOpen)
                C(effMainsChanged)
                C(effSetProgram)
                C(effGetProgram)
                C(effSetProgramName)
                C(effGetProgramName)
                C(effSetBlockSize)
                C(effEditGetRect)
                C(effEditOpen)
                C(effEditClose)
                C(effEditIdle)
                C(effGetChunk)
                C(effSetChunk)
                C(effProcessEvents)
                C(effString2Parameter)
                C(effGetProgramNameIndexed)
                C(effGetInputProperties)
                C(effGetOutputProperties)
                C(effGetPlugCategory)
                C(effOfflineNotify)
                C(effOfflinePrepare)
                C(effOfflineRun)
                C(effGetVendorVersion)
                C(effVendorSpecific)
                C(effProcessVarIo)
                C(effSetSpeakerArrangement)
                C(effSetBypass)
                C(effGetEffectName)
                C(effGetTailSize)
                C(effCanDo)

                C(effEditKeyDown)
                C(effEditKeyUp)
                C(effSetEditKnobMode)
                C(effGetMidiProgramName)
                C(effGetCurrentMidiProgram)
                C(effGetMidiProgramCategory)
                C(effHasMidiProgramsChanged)
                C(effGetMidiKeyName)
                C(effBeginSetProgram)
                C(effEndSetProgram)

                C(effGetSpeakerArrangement)
                C(effShellGetNextPlugin)
                C(effStartProcess)
                C(effStopProcess)
                C(effSetTotalSampleToProcess)
                C(effSetPanLaw)
                C(effBeginLoadBank)
                C(effBeginLoadProgram)

                C(effSetProcessPrecision)
                C(effGetNumMidiInputChannels)
                C(effGetNumMidiOutputChannels)

                // DEPRECATED STUFF
                C(effGetVu)
                C(effEditDraw)
                C(effEditMouse)
                C(effEditKey)
                C(effEditTop)
                C(effEditSleep)
                C(effIdentify)
                C(effGetNumProgramCategories)
                C(effCopyProgram)
                C(effConnectInput)
                C(effConnectOutput)
                C(effGetCurrentPosition)
                C(effGetDestinationBuffer)
                C(effSetBlockSizeAndSampleRate)
                C(effGetErrorText)
                C(effIdle)
                C(effGetIcon)
                C(effSetViewPosition)
                C(effKeysRequired)

                default:
                    r = "unknown";
                    break;
            }
        #undef C
        #undef D
            return r;
        }
    #endif /* LSP_DEBUG */

        VstIntPtr get_category(const int *classes)
        {
            VstIntPtr result    = kPlugCategUnknown;

            while ((classes != NULL) && ((*classes) >= 0))
            {
                switch (*classes)
                {
                    case meta::C_DELAY:
                    case meta::C_REVERB:
                        result = kPlugCategRoomFx;
                        break;

                    case meta::C_GENERATOR:
                    case meta::C_CONSTANT:
                    case meta::C_OSCILLATOR:
                    case meta::C_ENVELOPE:
                        result = kPlugCategGenerator;
                        break;

                    case meta::C_INSTRUMENT:
                    case meta::C_DRUM:
                    case meta::C_EXTERNAL:
                    case meta::C_PIANO:
                    case meta::C_SAMPLER:
                    case meta::C_SYNTH:
                    case meta::C_SYNTH_SAMPLER:
                        result = kPlugCategSynth;
                        break;

                    case meta::C_DISTORTION:
                    case meta::C_WAVESHAPER:
                    case meta::C_AMPLIFIER:
                    case meta::C_FILTER:
                    case meta::C_ALLPASS:
                    case meta::C_BANDPASS:
                    case meta::C_COMB:
                    case meta::C_EQ:
                    case meta::C_MULTI_EQ:
                    case meta::C_PARA_EQ:
                    case meta::C_HIGHPASS:
                    case meta::C_LOWPASS:
                    case meta::C_MODULATOR:
                    case meta::C_CHORUS:
                    case meta::C_FLANGER:
                    case meta::C_PHASER:
                    case meta::C_SPECTRAL:
                    case meta::C_PITCH:
                    case meta::C_MIXER:
                        result = kPlugCategEffect;
                        break;

                    case meta::C_UTILITY:
                    case meta::C_ANALYSER:
                        result = kPlugCategAnalysis;
                        break;

                    case meta::C_DYNAMICS:
                    case meta::C_COMPRESSOR:
                    case meta::C_EXPANDER:
                    case meta::C_GATE:
                    case meta::C_LIMITER:
                        result = kPlugCategMastering;
                        break;

                    case meta::C_SPATIAL:
                        result = kPlugCategSpacializer;
                        break;

                    case meta::C_FUNCTION:
                    case meta::C_SIMULATOR:
                    case meta::C_CONVERTER:
                        result = kPlugCategRestoration;
                        break;

                    // NOT SUPPORTED
    //                    result = kPlugSurroundFx;            ///< Dedicated surround processor
    //                    break;
    //
    //                    result = kPlugCategOfflineProcess;   ///< Offline Process
    //                    break;
    //
    //                    result = kPlugCategShell;            ///< Plug-in is container of other plug-ins  @see effShellGetNextPlugin
    //                    break;
                    default:
                        break;
                }


                if (result != kPlugCategUnknown)
                    break;

                classes++;
            }

            return result;
        }

        VstIntPtr VSTCALLBACK dispatcher(AEffect* e, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
        {
            VstIntPtr v = 0;

        #ifdef LSP_TRACE
            switch (opcode)
            {
                case effEditIdle:
                case effIdle:
                case effGetProgram:
                case effProcessEvents:
                case effGetTailSize:
                    break;
                default:
                    lsp_trace("vst_dispatcher effect=%p, opcode=%d (%s), index=%d, value=%llx, ptr=%p, opt = %.5f",
                        e, opcode, decode_opcode(opcode), index, (long long)(value), ptr, opt);
                    break;
            }
        #endif /* LSP_TRACE */

            // Get VST object
            vst2::Wrapper *w    = reinterpret_cast<vst2::Wrapper *>(e->object);

            switch (opcode)
            {
                case effGetVstVersion: // Get VST version of plugin
                    lsp_trace("vst_version = %d", int(kVstVersion));
                    v   = kVstVersion;
                    break;

                case effClose: // Finalize the plugin
                    if (e != NULL)
                        vst2::finalize(e);
                    v = 1;
                    break;

                case effGetVendorString: // Get vendor string
                {
                    const meta::package_t *package = w->package();
                    if (package != NULL)
                    {
                        char *dst = reinterpret_cast<char *>(ptr);
                        snprintf(dst, kVstMaxProductStrLen, "%s VST", package->brand);
                        dst[kVstMaxVendorStrLen - 1] = '\0';
                        lsp_trace("vendor_string = %s", reinterpret_cast<char *>(ptr));
                        v = 1;
                    }
                    break;
                }

                case effGetEffectName: // Get effect name
                {
                    const meta::plugin_t *m = w->metadata();
                    if (m != NULL)
                    {
                        char *dst = reinterpret_cast<char *>(ptr);
                        vst_strncpy(dst, m->vst2_name, kVstMaxEffectNameLen);
                        dst[kVstMaxEffectNameLen - 1] = '\0';
                        lsp_trace("effect_string = %s", dst);
                        v = 1;
                    }
                    break;
                }

                case effGetProductString: // Get product string
                {
                    const meta::plugin_t *m = w->metadata();
                    const meta::package_t *package = w->package();
                    if ((m != NULL) && (package != NULL))
                    {
                        char *dst = reinterpret_cast<char *>(ptr);
                        snprintf(dst, kVstMaxProductStrLen, "%s %s [VST]", package->brand, m->description);
                        dst[kVstMaxProductStrLen - 1] = '\0';
                        lsp_trace("product_string = %s", dst);
                        v = 1;
                    }
                    break;
                }

                case effGetParamName: // Get parameter name
                case effGetParamLabel: // Get units of the parameter
                case effGetParamDisplay: // Get value of the parameter
                {
                    vst2::ParameterPort *p  = w->parameter_port(index);
                    if (p == NULL)
                        break;

                    const meta::port_t *m = p->metadata();
                    if (m == NULL)
                        break;

                    char *dst = reinterpret_cast<char *>(ptr);

                    if (opcode == effGetParamName)
                    {
                        vst_strncpy(dst, m->id, kVstMaxParamStrLen);
                        lsp_trace("param_name = %s", dst);
                        if (strcmp(dst, m->id) != 0)
                            lsp_warn("parameter name was trimmed from %s to %s !!!", m->id, dst);
                    }
                    else if (opcode == effGetParamLabel)
                    {
                        const char *label = meta::get_unit_name((is_decibel_unit(m->unit)) ? meta::U_DB : m->unit);
                        if (label != NULL)
                            vst_strncpy(dst, label, kVstMaxParamStrLen);
                        else
                            dst[0] = '\0';
                        lsp_trace("param_label = %s", dst);
                    }
                    else
                    {
                        meta::format_value(dst, kVstMaxParamStrLen, m, p->value(), -1, false);
                        lsp_trace("param_display = %s", dst);
                    }
                    v = 1;
                    break;
                }

                case effCanBeAutomated:
                case effGetParameterProperties: // Parameter properties
                {
                    vst2::ParameterPort *p  = w->parameter_port(index);
                    if (p == NULL)
                        break;

                    const meta::port_t *m = p->metadata();
                    if (m == NULL)
                        break;

                    if (opcode == effGetParameterProperties)
                    {
                        get_parameter_properties(m, reinterpret_cast<VstParameterProperties *>(ptr));
                        v = 1;
                    }
                    else if (!(m->flags & meta::F_OUT))
                        v = 1;

                    break;
                }

                case effSetBlockSizeAndSampleRate: // Set block size and sample rate
                    w->set_block_size(value);
                    w->set_sample_rate(opt);
                    break;

                case effSetBlockSize: // Set block size
                    w->set_block_size(value);
                    break;

                case effSetSampleRate: // Set sample rate, always in suspended mode
                    w->set_sample_rate(opt);
                    break;

                case effOpen: // Plugin initialization
                    w->open();
                    break;

                case effMainsChanged: // Plugin activation/deactivation
                    w->mains_changed(value);
                    break;

                case effGetPlugCategory:
                {
                    const meta::plugin_t *m = w->metadata();
                    if (m == NULL)
                        break;

                    v = get_category(m->classes);
                    lsp_trace("plugin_category = %d", int(v));
                    break;
                }

            #ifndef LSP_NO_VST_UI
                case effEditOpen: // Run editor
                {
                    UIWrapper *ui = w->ui_wrapper();
                    if (ui == NULL)
                    {
                        if ((ui = UIWrapper::create(w, ptr)) == NULL)
                            break;
                    }

                    if (ui->show_ui())
                    {
                        w->set_ui_wrapper(ui);
                        v = 1;
                    }
                    break;
                }

                case effEditClose: // Close editor
                {
                    UIWrapper *ui = w->ui_wrapper();
                    if (ui == NULL)
                        break;

                    w->set_ui_wrapper(NULL);
                    ui->hide_ui();
                    ui->destroy();
                    delete ui;
                    v = 1;
                    break;
                }

                case effEditIdle: // Run editor's iteration
                {
                    UIWrapper *ui = w->ui_wrapper();
                    if (ui == NULL)
                        break;

                    ui->idle_ui();
                    v = 1;
                    break;
                }

                case effEditGetRect: // Return UI dimensions
                {
                    UIWrapper *ui = w->ui_wrapper();
                    if (ui == NULL)
                        break;

                    ERect **er = reinterpret_cast<ERect **>(ptr);
                    *er = ui->ui_rect();
                    lsp_trace("Edit rect = {%d, %d, %d, %d}", int((*er)->left), int((*er)->top), int((*er)->right), int((*er)->bottom));
                    v = 1;
                    break;
                }

                case effEditKeyDown:
                case effEditKeyUp:
                {
                    UIWrapper *uiw = w->ui_wrapper();
                    if (uiw == NULL)
                        break;

                    // Generate key event handler and pass it to the plugin
                    v = process_key_event(uiw, opcode, index, value);
                    break;
                }
            #endif /* LSP_NO_VST_UI */

                case effSetProgram:
                case effGetProgram:
                case effSetProgramName:
                case effGetProgramName:
                    break;

                case effGetChunk:
                    v       = w->serialize_state(reinterpret_cast<const void **>(ptr), index);
                    break;

                case effSetChunk:
                    if (e->flags & effFlagsProgramChunks)
                    {
                        w->deserialize_state(ptr, value);
                        v = 1;
                    }
                    break;

                case effProcessEvents:
                    w->process_events(reinterpret_cast<const VstEvents *>(ptr));
                    v = 1;
                    break;

                case effString2Parameter:
                case effGetProgramNameIndexed:
                case effGetInputProperties:
                case effGetOutputProperties:
                case effOfflineNotify:
                case effOfflinePrepare:
                case effOfflineRun:
                    break;

                case effGetVendorVersion:
                {
                    const meta::plugin_t *m = w->metadata();
                    if (m != NULL)
                        v = vst2::version(m->version);
                    break;
                }

                case effVendorSpecific:
                    // Check that we can provide VST3 compatibility information
                    if (((index == VST2_MAGIC_stCA) || (index == VST2_MAGIC_stCa)) &&
                        (value == VST2_MAGIC_FUID) && (ptr != NULL))
                    {
                        char FUID[40];
                        const meta::plugin_t *m = w->metadata();

                        if ((m != NULL) && (m->vst3_uid != NULL))
                        {
                            const char *plugin_name = (m->vst2_name != NULL) ? m->vst2_name : m->name;
                            if (meta::uid_vst2_to_vst3(FUID, m->vst2_uid, plugin_name) != NULL)
                            {
                                lsp_trace("Reporting compatibility of VST 2.x plugin uid='%s' with VST3.x plugin uuid='%s'", m->vst2_uid, FUID);
                                meta::uid_vst3_to_tuid(reinterpret_cast<char *>(ptr), FUID);
                                v   = 1;
                            }
                        }
                    }
                    break;

                case effProcessVarIo:
                case effSetSpeakerArrangement:
                case effGetTailSize:
                    break;

                case effSetBypass:
                    w->set_bypass(v);
                    break;

                case effCanDo:
                {
                    const char *text    = reinterpret_cast<const char *>(ptr);
                    lsp_trace("effCanDo request: %s\n", text);
                    if (e->flags & effFlagsIsSynth)
                    {
                        if (!::strcmp(text, canDoReceiveVstEvents))
                            v = 1;
                        else if (!::strcmp(text, canDoReceiveVstMidiEvent))
                            v = 1;
                        else if (!::strcmp(text, canDoSendVstEvents))
                            v = 1;
                        else if (!::strcmp(text, canDoSendVstMidiEvent))
                            v = 1;
                        else if (!::strcmp(text, canDoBypass))
                            v = w->has_bypass();
                    }
                    break;
                }

                case effSetEditKnobMode:

                case effGetMidiProgramName:
                case effGetCurrentMidiProgram:
                case effGetMidiProgramCategory:
                case effHasMidiProgramsChanged:
                case effGetMidiKeyName:

                case effBeginSetProgram:
                case effEndSetProgram:
                    break;

                case effGetSpeakerArrangement:
                case effShellGetNextPlugin:

                case effStartProcess:
                case effStopProcess:
                case effSetTotalSampleToProcess:
                case effSetPanLaw:

                case effBeginLoadBank:
                case effBeginLoadProgram:
                    break;

                case effSetProcessPrecision:    // Currently no double-precision processing supported
                    v   = 0;
                    break;
                case effGetNumMidiInputChannels:
                case effGetNumMidiOutputChannels:
                    break;

                // DEPRECATED STUFF
                case effIdentify:
                    v = kEffectIdentify;
                    break;

                case effGetVu:
                case effEditDraw:
                case effEditMouse:
                case effEditKey:
                case effEditTop:
                case effEditSleep:
                case effGetNumProgramCategories:
                case effCopyProgram:
                case effConnectInput:
                case effConnectOutput:
                case effGetCurrentPosition:
                case effGetDestinationBuffer:
                case effGetErrorText:
                case effIdle:
                case effGetIcon:
                case effSetViewPosition:
                case effKeysRequired:
                    break;

                default:
                    break;
            }

        #ifdef LSP_TRACE
            switch (opcode)
            {
                case effEditKeyDown:
                case effEditKeyUp:
                    lsp_trace("vst_dispatcher return v=%d", int(v));
                    break;
            }
        #endif /* LSP_TRACE */

            return v;
        }

        void VSTCALLBACK process(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames)
        {
    //        lsp_trace("vst_process effect=%p, inputs=%p, outputs=%p, frames=%d", effect, inputs, outputs, int(sampleFrames));
            dsp::context_t ctx;
            vst2::Wrapper *w        = reinterpret_cast<vst2::Wrapper *>(effect->object);

            // Call the plugin for processing
            dsp::start(&ctx);
            w->run_legacy(inputs, outputs, sampleFrames);
            dsp::finish(&ctx);
        }

        void VSTCALLBACK process_replacing(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames)
        {
    //        lsp_trace("vst_process effect=%p, inputs=%p, outputs=%p, frames=%d", effect, inputs, outputs, int(sampleFrames));
            dsp::context_t ctx;
            vst2::Wrapper *w        = reinterpret_cast<vst2::Wrapper *>(effect->object);

            // Call the plugin for processing
            dsp::start(&ctx);
            w->run(inputs, outputs, sampleFrames);
            dsp::finish(&ctx);
        }

        void VSTCALLBACK set_parameter(AEffect* effect, VstInt32 index, float value)
        {
            lsp_trace("vst_set_parameter effect=%p, index=%d, value=%.3f", effect, int(index), value);

            // Get VST object
            vst2::Wrapper *w        = reinterpret_cast<vst2::Wrapper *>(effect->object);
            if (w == NULL)
                return;

            // Get VST parameter port
            vst2::ParameterPort *vp = w->parameter_port(index);
            if (vp != NULL)
                vp->set_vst_value(value);
        }

        float VSTCALLBACK get_parameter(AEffect* effect, VstInt32 index)
        {
    //        lsp_trace("vst_get_parameter effect=%p, index=%d", effect, int(index));

            // Get VST object
            vst2::Wrapper *w        = reinterpret_cast<vst2::Wrapper *>(effect->object);
            if (w == NULL)
                return 0.0f;

            // Get port and apply parameter
            vst2::ParameterPort *vp = w->parameter_port(index);
            return (vp != NULL) ? vp->vst_value() : 0.0f;
        }

        AEffect *instantiate(const char *uid, audioMasterCallback callback)
        {
            // Initialize debug
        #ifndef LSP_IDE_DEBUG
            IF_DEBUG( debug::redirect(VST2_LOG_FILE); );
        #endif /* LSP_IDE_DEBUG */
            lsp_trace("uid=%s, callback=%p", uid, callback);

            // Initialize DSP
            dsp::init();

            // Lookup plugin identifier among all registered plugin factories
            plug::Module *plugin = NULL;

            for (plug::Factory *f = plug::Factory::root(); (plugin == NULL) && (f != NULL); f = f->next())
            {
                for (size_t i=0; plugin == NULL; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;
                    if ((plug_meta->uid == NULL) ||
                        (plug_meta->vst2_uid == NULL))
                        continue;

                    // Check plugin identifier
                    if (!strcmp(plug_meta->vst2_uid, uid))
                    {
                        // Instantiate the plugin and return
                        if ((plugin = f->create(plug_meta)) == NULL)
                        {
                            lsp_error("Plugin instantiation error: '%s' ('%s')", plug_meta->uid, plug_meta->vst2_uid);
                            return NULL;
                        }
                    }
                }
            }

            // Check that plugin instance is available
            if (plugin == NULL)
            {
                lsp_error("Unknown plugin identifier: '%s'", uid);
                return NULL;
            }

            const meta::plugin_t *meta = plugin->metadata();
            lsp_trace("Instantiated plugin %s - %s", meta->name, meta->description);

            // Create effect descriptor
            AEffect *e                  = new AEffect;
            if (e == NULL)
            {
                delete plugin;
                return NULL;
            }

            // Create resource loader
            resource::ILoader *loader = core::create_resource_loader();
            if (loader != NULL)
            {
                vst2::Wrapper *wrapper  = new vst2::Wrapper(plugin, loader, e, callback);
                if (wrapper != NULL)
                {
                    // These objects will be automatically destroyed by the wrapper->destroy()
                    plugin = NULL;
                    loader = NULL;

                    // Initialize effect structure
                    ::bzero(e, sizeof(AEffect));

                    // Fill effect with values depending on metadata
                    e->magic                            = kEffectMagic;
                    e->dispatcher                       = vst2::dispatcher;
                    e->process                          = vst2::process;
                    e->setParameter                     = vst2::set_parameter;
                    e->getParameter                     = vst2::get_parameter;
                    e->numPrograms                      = 0;
                    e->numParams                        = 0;
                    e->numInputs                        = 0;
                    e->numOutputs                       = 0;
                    e->flags                            = effFlagsCanReplacing;
                    e->initialDelay                     = 0;
                    e->object                           = wrapper;
                    e->user                             = NULL;
                    e->uniqueID                         = vst2::cconst(meta->vst2_uid);
                    e->version                          = vst2::version(meta->version);
                    e->processReplacing                 = vst2::process_replacing;
                    e->processDoubleReplacing           = NULL; // Currently no double-replacing

                    // Additional flags
                    if (meta->ui_resource != NULL)
                        e->flags                        |= effFlagsHasEditor; // Has custom UI

                    status_t res = wrapper->init();
                    if (res == STATUS_OK)
                        return e;

                    lsp_error("Error initializing plugin wrapper, code: %d", int(res));
                }
                else
                    lsp_error("Error allocating plugin wrapper");

                if (loader != NULL)
                    delete loader;
            }
            else
                lsp_error("No resource loader available");

            if (plugin != NULL)
                delete plugin;
            vst2::finalize(e);

            // Plugin could not be instantiated
            return NULL;
        }

    } /* namespace vst2 */
} /* namespace lsp */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    LSP_EXPORT_MODIFIER
    AEffect *VST_MAIN_FUNCTION(const char *plugin_vst2_id, audioMasterCallback callback)
    {
        return lsp::vst2::instantiate(plugin_vst2_id, callback);
    }
#ifdef __cplusplus
}
#endif /* __cplusplus */
