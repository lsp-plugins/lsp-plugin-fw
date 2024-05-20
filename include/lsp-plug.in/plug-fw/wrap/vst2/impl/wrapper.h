/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/wrapper.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/defs.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/helpers.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/ipc/NativeExecutor.h>

namespace lsp
{
    namespace vst2
    {
        Wrapper::Wrapper(
            plug::Module *plugin,
            resource::ILoader *loader,
            AEffect *effect,
            audioMasterCallback callback)
            : plug::IWrapper(plugin, loader)
        {
            pPlugin         = plugin;
            pEffect         = effect;

            pMaster         = callback;
            pExecutor       = NULL;

            fLatency        = 0.0f;
            nDumpReq        = 0;
            nDumpResp       = 0;

            pSamplePlayer   = NULL;

            pBypass         = NULL;
            bUpdateSettings = true;
            bStateManage    = false;
            pUIWrapper      = NULL;
            nUIReq          = 0;
            nUIResp         = 0;
            pPackage        = NULL;
        }

        Wrapper::~Wrapper()
        {
            pPlugin         = NULL;
            pEffect         = NULL;
            pMaster         = NULL;
            pPackage        = NULL;
        }

        static ssize_t cmp_port_identifiers(const vst2::Port *pa, const vst2::Port *pb)
        {
            const meta::port_t *a = pa->metadata();
            const meta::port_t *b = pb->metadata();
            return strcmp(a->id, b->id);
        }

        status_t Wrapper::init()
        {
            AEffect *e                      = pEffect;
            const meta::plugin_t *m         = pPlugin->metadata();

            status_t res;

            // Load package information
            io::IInStream *is = resources()->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is == NULL)
            {
                lsp_error("No manifest.json found in resources");
                return STATUS_BAD_STATE;
            }

            res = meta::load_manifest(&pPackage, is);
            is->close();
            delete is;

            if (res != STATUS_OK)
            {
                lsp_error("Error while reading manifest file");
                return res;
            }

            // Create ports
            lsp_trace("Creating ports for %s - %s", m->name, m->description);
            lltl::parray<plug::IPort> plugin_ports;
            for (const meta::port_t *port = m->ports ; port->id != NULL; ++port)
                create_port(&plugin_ports, port, NULL);

            if (!vSortedPorts.add(&vPorts))
                return STATUS_NO_MEM;
            vSortedPorts.qsort(cmp_port_identifiers);

            // Get buffer size
            ssize_t blk_size = pMaster(pEffect, audioMasterGetBlockSize, 0, 0, 0, 0);
            if (blk_size > 0)
                set_block_size(blk_size);

            // Update instance parameters
            e->numInputs                    = 0;
            e->numOutputs                   = 0;
            e->numParams                    = vParams.size();

            for (size_t i=0, n=vAudioPorts.size(); i<n; ++i)
            {
                vst2::Port *port = vAudioPorts.uget(i);
                if (meta::is_in_port(port->metadata()))
                    ++e->numInputs;
                else
                    ++e->numOutputs;
            }


            // Generate IDs for parameter ports
            for (ssize_t id=0; id < e->numParams; ++id)
                vParams[id]->set_id(id);

            // Initialize state chunk
            pEffect->flags                 |= effFlagsProgramChunks;

            // Initialize plugin
            pPlugin->init(this, plugin_ports.array());

            // Create sample player if required
            if (m->extensions & meta::E_FILE_PREVIEW)
            {
                pSamplePlayer       = new core::SamplePlayer(m);
                if (pSamplePlayer == NULL)
                    return STATUS_NO_MEM;
                pSamplePlayer->init(this, plugin_ports.array(), plugin_ports.size());
            }

            return STATUS_OK;
        }

        void Wrapper::destroy()
        {
            // Destroy sample player
            if (pSamplePlayer != NULL)
            {
                pSamplePlayer->destroy();
                delete pSamplePlayer;
                pSamplePlayer = NULL;
            }

            // Shutdown and delete executor if exists
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                delete pExecutor;
                pExecutor   = NULL;
            }

            // Destrop plugin
            lsp_trace("destroying plugin");
            if (pPlugin != NULL)
            {
                pPlugin->destroy();
                delete pPlugin;

                pPlugin = NULL;
            }

            // Destroy ports
            for (size_t i=0; i<vPorts.size(); ++i)
            {
                lsp_trace("destroy port id=%s", vPorts[i]->metadata()->id);
                delete vPorts[i];
            }
            vPorts.clear();

            // Cleanup generated metadata
            for (size_t i=0; i<vGenMetadata.size(); ++i)
            {
                meta::port_t *p = vGenMetadata.uget(i);
                lsp_trace("destroy generated port metadata %p", p);
                drop_port_metadata(p);
            }
            vGenMetadata.flush();

            // Destroy manifest
            if (pPackage != NULL)
            {
                meta::free_manifest(pPackage);
                pPackage        = NULL;
            }

            // Delete the loader
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader = NULL;
            }

            // Clear all port lists
            vAudioPorts.clear();
            vParams.clear();

            pMaster     = NULL;
            pEffect     = NULL;

            lsp_trace("destroy complete");
        }

        vst2::Port *Wrapper::create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix)
        {
            vst2::Port *vp = NULL;

            switch (port->role)
            {
                case meta::R_MESH:
                    lsp_trace("creating mesh port %s", port->id);
                    vp  = new vst2::MeshPort(port, pEffect, pMaster);
                    plugin_ports->add(vp);
                    break;

                case meta::R_STREAM:
                    lsp_trace("creating stream port %s", port->id);
                    vp  = new vst2::StreamPort(port, pEffect, pMaster);
                    plugin_ports->add(vp);
                    break;

                case meta::R_FBUFFER:
                    lsp_trace("creating fbuffer port %s", port->id);
                    vp  = new vst2::FrameBufferPort(port, pEffect, pMaster);
                    plugin_ports->add(vp);
                    break;

                case meta::R_MIDI_OUT:
                    lsp_trace("creating midi output port %s", port->id);
                    vp = new vst2::MidiOutputPort(port, pEffect, pMaster);
                    plugin_ports->add(vp);
                    break;

                case meta::R_MIDI_IN:
                    pEffect->flags         |= effFlagsIsSynth;
                    vp = new vst2::MidiInputPort(port, pEffect, pMaster);
                    plugin_ports->add(vp);
                    break;

                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                    lsp_trace("creating osc port %s", port->id);
                    vp      = new vst2::OscPort(port, pEffect, pMaster);
                    break;

                case meta::R_PATH:
                    lsp_trace("creating path port %s", port->id);
                    vp  = new vst2::PathPort(port, pEffect, pMaster);
                    plugin_ports->add(vp);
                    break;

                case meta::R_AUDIO_IN:
                case meta::R_AUDIO_OUT:
                    lsp_trace("creating audio port %s", port->id);
                    vp = new vst2::AudioPort(port, pEffect, pMaster);
                    plugin_ports->add(vp);
                    vAudioPorts.add(static_cast<vst2::AudioPort *>(vp));
                    break;

                case meta::R_CONTROL:
                    lsp_trace("creating control port %s", port->id);
                    vp      = new vst2::ParameterPort(port, pEffect, pMaster);
                    if (postfix == NULL)
                        vParams.add(static_cast<vst2::ParameterPort *>(vp));
                    plugin_ports->add(vp);
                    break;

                case meta::R_BYPASS:
                    lsp_trace("creating bypass port %s", port->id);
                    vp      = new vst2::ParameterPort(port, pEffect, pMaster);
                    if (postfix == NULL)
                        vParams.add(static_cast<vst2::ParameterPort *>(vp));
                    pBypass     = vp;
                    plugin_ports->add(vp);
                    break;

                case meta::R_METER:
                    lsp_trace("creating meter port %s", port->id);
                    vp      = new vst2::MeterPort(port, pEffect, pMaster);
                    plugin_ports->add(vp);
                    break;

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    vst2::PortGroup *pg         = new vst2::PortGroup(port, pEffect, pMaster);

                    // Add immediately to port list
                    lsp_trace("creating port_set port %s", port->id);
                    plugin_ports->add(pg);
                    vPorts.add(pg);

                    // Add nested ports
                    for (size_t row=0; row<pg->rows(); ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Clone port metadata
                        meta::port_t *cm        = clone_port_metadata(port->members, postfix_buf);
                        if (cm != NULL)
                        {
                            vGenMetadata.add(cm);

                            for (; cm->id != NULL; ++cm)
                            {
                                if (meta::is_growing_port(cm))
                                    cm->start    = cm->min + ((cm->max - cm->min) * row) / float(pg->rows());
                                else if (meta::is_lowering_port(cm))
                                    cm->start    = cm->max - ((cm->max - cm->min) * row) / float(pg->rows());

                                // Create recursively all nested ports
                                create_port(plugin_ports, cm, postfix_buf);
                            }
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            if (vp != NULL)
                vPorts.add(vp);

            return vp;
        }

        void Wrapper::set_sample_rate(float sr)
        {
            if (sr > MAX_SAMPLE_RATE)
            {
                lsp_warn("Unsupported sample rate: %f, maximum supported sample rate is %ld", sr, long(MAX_SAMPLE_RATE));
                sr  = MAX_SAMPLE_RATE;
            }
            pPlugin->set_sample_rate(sr);
            if (pSamplePlayer != NULL)
                pSamplePlayer->set_sample_rate(sr);
            bUpdateSettings = true;
        }

        void Wrapper::set_block_size(size_t size)
        {
            lsp_trace("Block size for audio processing: %d", int(size));

            // Sync buffer size to all input ports
            for (size_t i=0, n=vAudioPorts.size(); i<n; ++i)
            {
                vst2::AudioPort *p = vAudioPorts.uget(i);
                if (p != NULL)
                    p->set_block_size(size);
            }
        }

        void Wrapper::mains_changed(VstIntPtr value)
        {
            if (value)
                pPlugin->activate();
            else
                pPlugin->deactivate();
        }

        bool Wrapper::has_bypass() const
        {
            return pBypass != NULL;
        }

        void Wrapper::set_bypass(bool bypass)
        {
            pBypass->write_value((bypass) ? 1.0f : 0.0f);
        }

        ipc::IExecutor *Wrapper::executor()
        {
            lsp_trace("executor = %p", reinterpret_cast<void *>(pExecutor));
            if (pExecutor != NULL)
                return pExecutor;

            lsp_trace("Creating native executor service");
            ipc::NativeExecutor *exec = new ipc::NativeExecutor();
            if (exec == NULL)
                return NULL;
            if (exec->start() != STATUS_OK)
            {
                delete exec;
                return NULL;
            }
            return pExecutor = exec;
        }

        vst2::ParameterPort *Wrapper::parameter_port(size_t index)
        {
            return vParams[index];
        }

        void Wrapper::open()
        {
        }

        void Wrapper::set_ui_wrapper(UIWrapper *ui)
        {
            if (pUIWrapper == ui)
                return;

            pUIWrapper  = ui;
            atomic_add(&nUIReq, 1);
        }

        UIWrapper *Wrapper::ui_wrapper()
        {
            return pUIWrapper;
        }

        void Wrapper::sync_position()
        {
            VstTimeInfo *info   = FromVstPtr<VstTimeInfo>(pMaster(pEffect, audioMasterGetTime, 0, kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstCyclePosValid | kVstTimeSigValid, NULL, 0.0f));
            if (info == NULL)
                return;

            plug::position_t npos   = sPosition;

            npos.sampleRate         = info->sampleRate;
            npos.speed              = 1.0f;
            npos.ticksPerBeat       = DEFAULT_TICKS_PER_BEAT;
            npos.frame              = info->samplePos;

    //        lsp_trace("info->flags          = 0x%08x", int(info->flags));
    //        lsp_trace("info->sampleRate     = %f", info->sampleRate);
    //        lsp_trace("info->samplePos      = %f", info->samplePos);
    //        lsp_trace("info->numerator      = %d", int(info->timeSigNumerator));
    //        lsp_trace("info->denominator    = %d", int(info->timeSigDenominator));
    //        lsp_trace("info->bpm            = %f", info->tempo);

            if (info->flags & kVstTimeSigValid)
            {
                npos.numerator      = info->timeSigNumerator;
                npos.denominator    = info->timeSigDenominator;

    //            lsp_trace("ppq_pos = %f, bar_start_pos = %f", float(info->ppqPos), float(info->barStartPos));
                if ((info->flags & (kVstPpqPosValid | kVstBarsValid)) == (kVstPpqPosValid | kVstBarsValid))
                {
                    double uppqPos      = (info->ppqPos - info->barStartPos) * info->timeSigDenominator * 0.25 / npos.numerator;
                    npos.tick           = npos.ticksPerBeat * npos.numerator * (uppqPos - int64_t(uppqPos));
                }
            }

            if (info->flags & kVstTempoValid)
                npos.beatsPerMinute = info->tempo;

    //        lsp_trace("position: sr=%f, frame=%ld, key=%f/%f tick=%f bpm=%f",
    //                float(npos.sampleRate), long(npos.frame), float(npos.numerator),
    //                float(npos.denominator), float(npos.tick), float(npos.beatsPerMinute));

            // Report new position to plugin and update current position
            if (pPlugin->set_position(&npos))
                bUpdateSettings = true;
            sPosition       = npos;

//            lsp_trace("position sampleRate=%f, speed=%f, num=%f, den=%f, bpm=%f, tpb=%f, tick=%f",
//                npos.sampleRate, npos.speed, npos.numerator, npos.denominator, npos.beatsPerMinute, npos.ticksPerBeat, npos.tick);
        }

        void Wrapper::run(float** inputs, float** outputs, size_t samples)
        {
            // DO NOTHING if sample_rate is not set (fill output buffers with zeros)
            if (pPlugin->get_sample_rate() <= 0)
            {
                for (size_t i=0, n=vAudioPorts.size(); i < n; ++i)
                {
                    vst2::AudioPort *port = vAudioPorts.uget(i);
                    if (meta::is_audio_out_port(port->metadata()))
                        dsp::fill_zero(*(outputs++), samples);
                }
                return;
            }

            // Sync UI state
            const uatomic_t ui_req = nUIReq;
            if (ui_req != nUIResp)
            {
                if (pPlugin->ui_active())
                    pPlugin->deactivate_ui();
                if (pUIWrapper != NULL)
                    pPlugin->activate_ui();
                nUIResp     = ui_req;
            }

            // Synchronize position
            sync_position();

            // Bind input audio data and sanitize ports
            for (size_t i=0, n=vAudioPorts.size(); i<n; ++i)
            {
                vst2::AudioPort *port = vAudioPorts.uget(i);
                if (port == NULL)
                    continue;

                float *data = (meta::is_audio_in_port(port->metadata())) ? *(inputs++) : *(outputs++);
                port->bind(data);
                port->sanitize_before(samples);
            }

            // Process ALL ports for changes
            size_t n_ports      = vPorts.size();
            vst2::Port **v_ports= vPorts.array();

            for (size_t i=0; i<n_ports; ++i)
            {
                // Get port
                vst2::Port *port = v_ports[i];
                if (port == NULL)
                    continue;

                // Pre-process data in port
                if (port->pre_process(samples))
                {
                    lsp_trace("port changed: %s", port->metadata()->id);
                    bUpdateSettings = true;
                }
            }

            // Check that input parameters have changed
            if (bUpdateSettings)
            {
                lsp_trace("updating settings");
                pPlugin->update_settings();
                bUpdateSettings     = false;
            }

            // Need to dump state?
            uatomic_t dump_req  = nDumpReq;
            if (dump_req != nDumpResp)
            {
                dump_plugin_state();
                nDumpResp           = dump_req;
            }

            // Process samples
            pPlugin->process(samples);

            // Launch the sample player
            if (pSamplePlayer != NULL)
                pSamplePlayer->process(samples);

            // Sanitize output audio data
            for (size_t i=0, n=vAudioPorts.size(); i<n; ++i)
            {
                vst2::AudioPort *port = vAudioPorts.uget(i);
                if (port != NULL)
                    port->sanitize_after(samples);
            }

            // Report latency
            float latency           = pPlugin->latency();
            if (fLatency != latency)
            {
                pEffect->initialDelay   = latency;
                fLatency                = latency;
                if (pMaster)
                {
                    lsp_trace("Reporting latency = %d samples to the host", int(latency));
                    pMaster(pEffect, audioMasterIOChanged, 0, 0, 0, 0);
                }
            }

            // Post-process ALL ports
            for (size_t i=0; i<n_ports; ++i)
            {
                vst2::Port *port = v_ports[i];
                if (port != NULL)
                    port->post_process(samples);
            }
        }

        void Wrapper::process_events(const VstEvents *e)
        {
            // We need to deliver MIDI events to MIDI ports
            for (size_t i=0; i<vPorts.size(); ++i)
            {
                vst2::Port *p               = vPorts[i];

                // Find MIDI port(s)
                if (!meta::is_midi_in_port(p->metadata()))
                    continue;

                // Call for event processing
                vst2::MidiInputPort *mp     = static_cast<vst2::MidiInputPort *>(p);
                mp->deserialize(e);
            }
        }

        void Wrapper::run_legacy(float** inputs, float** outputs, size_t samples)
        {
            run(inputs, outputs, samples);
        }

        status_t Wrapper::serialize_port_data()
        {
            size_t param_off    = 0;

            // Serialize all regular ports
            for (size_t i=0; i<vPorts.size(); ++i)
            {
                // Get VST port
                vst2::Port *vp             = vPorts[i];
                if (vp == NULL)
                    continue;

                // Get metadata
                const meta::port_t *p      = vp->metadata();
                if ((p == NULL) || (p->id == NULL) || (meta::is_out_port(p)) || (!vp->serializable()))
                    continue;

                // Check that port is serializable
                lsp_trace("Serializing port id=%s", p->id);

                // Write port data to the chunk
                param_off   = sChunk.write(uint32_t(0)); // Reserve space for size
                sChunk.write_string(p->id);     // ID of the port
                vp->serialize(&sChunk);         // Value of the port
                sChunk.write_at(param_off, uint32_t(sChunk.offset - param_off - sizeof(uint32_t))); // Write the actual size

                if (sChunk.res != STATUS_OK)
                {
                    lsp_warn("Error serializing parameter is=%s, code=%d", p->id, int(sChunk.res));
                    return sChunk.res;
                }
            }

            status_t res = STATUS_OK;

            // Serialize KVT storage
            if (sKVTMutex.lock())
            {
                const core::kvt_param_t *p;

                // Read the whole KVT storage
                core::KVTIterator *it = sKVT.enum_all();
                while (it->next() == STATUS_OK)
                {
                    res             = it->get(&p);
                    if (res == STATUS_NOT_FOUND) // Not a parameter
                        continue;
                    else if (res != STATUS_OK)
                    {
                        lsp_warn("it->get() returned %d", int(res));
                        break;
                    }
                    else if (it->is_transient()) // Skip transient parameters
                        continue;

                    const char *name = it->name();
                    if (name == NULL)
                    {
                        lsp_trace("it->name() returned NULL");
                        break;
                    }

                    uint8_t flags = 0;
                    if (it->is_private())
                        flags      |= vst2::FLAG_PRIVATE;

                    kvt_dump_parameter("Saving state of KVT parameter: %s = ", p, name);

                    param_off   = sChunk.write(uint32_t(0)); // Reserve space for size
                    sChunk.write_string(name); // Name of the KVT parameter
                    sChunk.write_byte(flags);

                    // Serialize parameter according to it's type
                    switch (p->type)
                    {
                        case core::KVT_INT32:
                        {
                            sChunk.write_byte(vst2::TYPE_INT32);
                            sChunk.write(p->i32);
                            break;
                        };
                        case core::KVT_UINT32:
                        {
                            sChunk.write_byte(vst2::TYPE_UINT32);
                            sChunk.write(p->u32);
                            break;
                        }
                        case core::KVT_INT64:
                        {
                            sChunk.write_byte(vst2::TYPE_INT64);
                            sChunk.write(p->i64);
                            break;
                        };
                        case core::KVT_UINT64:
                        {
                            sChunk.write_byte(vst2::TYPE_UINT64);
                            sChunk.write(p->u64);
                            break;
                        }
                        case core::KVT_FLOAT32:
                        {
                            sChunk.write_byte(vst2::TYPE_FLOAT32);
                            sChunk.write(p->f32);
                            break;
                        }
                        case core::KVT_FLOAT64:
                        {
                            sChunk.write_byte(vst2::TYPE_FLOAT64);
                            sChunk.write(p->f64);
                            break;
                        }
                        case core::KVT_STRING:
                        {
                            sChunk.write_byte(vst2::TYPE_STRING);
                            sChunk.write_string((p->str != NULL) ? p->str : "");
                            break;
                        }
                        case core::KVT_BLOB:
                        {
                            if ((p->blob.size > 0) && (p->blob.data == NULL))
                            {
                                res = STATUS_INVALID_VALUE;
                                break;
                            }

                            sChunk.write_byte(vst2::TYPE_BLOB);
                            sChunk.write_string((p->blob.ctype != NULL) ? p->blob.ctype : "");
                            if (p->blob.size > 0)
                                sChunk.write(p->blob.data, p->blob.size);
                            break;
                        }

                        default:
                            res     = STATUS_BAD_TYPE;
                            break;
                    }

                    // Successful status?
                    if (res != STATUS_OK)
                    {
                        lsp_trace("KVT serialization failed");
                        break;
                    }

                    // Complete the parameter size
                    sChunk.write_at(param_off, uint32_t(sChunk.offset - param_off - sizeof(uint32_t))); // Write the actual size
                }

                sKVT.gc();
                sKVTMutex.unlock();
            }

            return res;
        }

        size_t Wrapper::serialize_state(const void **dst, bool program)
        {
            // Set state manage barrier
            bStateManage = true;
            lsp_finally { bStateManage = false; };

            // Trigger the plugin to prepare the internal state.
            pPlugin->before_state_save();

            // Clear chunk
            status_t res;
            sChunk.clear();

            // Write the bank header
            if (program)
            {
                fxProgram prog;
                ::bzero(&prog, sizeof(prog));

                prog.chunkMagic     = CPU_TO_BE(VstInt32(cMagic));
                prog.byteSize       = 0;
                prog.fxMagic        = CPU_TO_BE(VstInt32(chunkPresetMagic));
                prog.version        = CPU_TO_BE(VstInt32(1));    // Version 1.0.0 of the program
                prog.fxID           = CPU_TO_BE(VstInt32(pEffect->uniqueID));
                prog.fxVersion      = CPU_TO_BE(VstInt32(VST_FX_VERSION_JUCE_FIX));
                prog.numParams      = 0;
                prog.prgName[0]     = '\0';

                size_t prog_off     = sChunk.write(&prog, offsetof(fxProgram, content.data.chunk));

                vst2::state_header_t hdr;
                ::bzero(&hdr, sizeof(hdr));
                hdr.nMagic1         = CPU_TO_BE(VstInt32(LSP_VST_USER_MAGIC));
                hdr.nSize           = 0;
                hdr.nVersion        = CPU_TO_BE(VstInt32(VST_FX_VERSION_JUCE_FIX));
                hdr.nMagic2         = CPU_TO_BE(VstInt32(LSP_VST_USER_MAGIC));
                size_t hdr_off      = sChunk.write(&hdr, sizeof(hdr));
                size_t data_off     = sChunk.offset;

                if ((res = serialize_port_data()) != STATUS_OK)
                {
                    *dst    = NULL;
                    return 0;
                }

                // Write the size of chunk
                fxProgram *pprog            = sChunk.fetch<fxProgram>(prog_off);
                VstInt32 size               = sChunk.offset - VST_PROGRAM_HDR_SKIP;
                pprog->content.data.size    = CPU_TO_BE(VstInt32(sChunk.offset - hdr_off));
                pprog->byteSize             = CPU_TO_BE(size);

                vst2::state_header_t *phdr  = sChunk.fetch<vst2::state_header_t>(hdr_off);
                phdr->nSize                 = CPU_TO_BE(VstInt32(sChunk.offset - data_off));

                dump_vst_bank(pprog, sChunk.offset);
                *dst                = pprog;
            }
            else
            {
                fxBank bank;
                ::bzero(&bank, sizeof(bank));

                bank.chunkMagic     = CPU_TO_BE(VstInt32(cMagic));
                bank.byteSize       = 0;
                bank.fxMagic        = CPU_TO_BE(VstInt32(chunkBankMagic));
                bank.version        = CPU_TO_BE(VstInt32(1)); // Version 2.0.0 of the bank
                bank.fxID           = CPU_TO_BE(VstInt32(pEffect->uniqueID));
                bank.fxVersion      = CPU_TO_BE(VstInt32(VST_FX_VERSION_JUCE_FIX)); // Version 2.0.0 of the bank
                bank.numPrograms    = 0;
                bank.currentProgram = 0;

                size_t bank_off     = sChunk.write(&bank, offsetof(fxBank, content.data.chunk));

                vst2::state_header_t hdr;
                ::bzero(&hdr, sizeof(hdr));
                hdr.nMagic1         = CPU_TO_BE(VstInt32(LSP_VST_USER_MAGIC));
                hdr.nSize           = 0;
                hdr.nVersion        = CPU_TO_BE(VstInt32(VST_FX_VERSION_JUCE_FIX));
                hdr.nMagic2         = CPU_TO_BE(VstInt32(LSP_VST_USER_MAGIC));
                size_t hdr_off      = sChunk.write(&hdr, sizeof(hdr));
                size_t data_off     = sChunk.offset;

                if ((res = serialize_port_data()) != STATUS_OK)
                {
                    *dst    = NULL;
                    return 0;
                }

                // Write the size of chunk
                fxBank *pbank               = sChunk.fetch<fxBank>(bank_off);
                VstInt32 size               = sChunk.offset - VST_BANK_HDR_SKIP;
                pbank->content.data.size    = CPU_TO_BE(VstInt32(sChunk.offset - hdr_off));
                pbank->byteSize             = CPU_TO_BE(size);

                vst2::state_header_t *phdr  = sChunk.fetch<vst2::state_header_t>(hdr_off);
                phdr->nSize                 = CPU_TO_BE(VstInt32(sChunk.offset - data_off));

                dump_vst_bank(pbank, sChunk.offset);
                *dst                = pbank;
            }

            // Notify the plugin that the state has been saved
            lsp_trace("Plugin state has been saved");
            pPlugin->state_saved();

            // Return result
            return sChunk.offset;
        }

        status_t Wrapper::check_vst_bank_header(const fxBank *bank, size_t size)
        {
            // Validate size
            if (size < size_t(offsetof(fxBank, content.data.chunk)))
            {
                lsp_warn("block size too small (0x%08x bytes)", int(size));
                return STATUS_NOT_FOUND;
            }

            // Validate chunkMagic
            if (bank->chunkMagic != BE_TO_CPU(cMagic))
            {
                lsp_warn("bank->chunkMagic (%08x) != BE_DATA(VST_CHUNK_MAGIC) (%08x)", int(bank->chunkMagic), int(BE_TO_CPU(cMagic)));
                return STATUS_NOT_FOUND;
            }

            // Validate fxMagic
            if (bank->fxMagic != BE_TO_CPU(chunkBankMagic))
            {
                lsp_warn("bank->fxMagic (%08x) != BE_DATA(VST_OPAQUE_BANK_MAGIC) (%08x)", int(bank->fxMagic), int(BE_TO_CPU(chunkBankMagic)));
                return STATUS_UNSUPPORTED_FORMAT;
            }

            // Validate fxID
            if (bank->fxID != BE_TO_CPU(VstInt32(pEffect->uniqueID)))
            {
                lsp_warn("bank->fxID (%08x) != BE_DATA(VstInt32(pEffect->uniqueID)) (%08x)", int(bank->fxID), int(BE_TO_CPU(VstInt32(pEffect->uniqueID))));
                return STATUS_UNSUPPORTED_FORMAT;
            }

            // Validate the numParams
            if (bank->numPrograms != 0)
            {
                lsp_warn("bank->numPrograms (%d) != 0", int(bank->numPrograms));
                return STATUS_UNSUPPORTED_FORMAT;
            }

            return STATUS_OK;
        }

        status_t Wrapper::check_vst_program_header(const fxProgram *prog, size_t size)
        {
            // Validate size
            if (size < size_t(offsetof(fxProgram, content.data.chunk)))
            {
                lsp_warn("block size too small (0x%08x bytes)", int(size));
                return STATUS_NOT_FOUND;
            }

            // Validate chunkMagic
            if (prog->chunkMagic != BE_TO_CPU(cMagic))
            {
                lsp_warn("prog->chunkMagic (%08x) != BE_DATA(VST_CHUNK_MAGIC) (%08x)", int(prog->chunkMagic), int(BE_TO_CPU(cMagic)));
                return STATUS_NOT_FOUND;
            }

            // Validate fxMagic
            if (prog->fxMagic != BE_TO_CPU(chunkPresetMagic))
            {
                lsp_warn("prog->fxMagic (%08x) != BE_DATA(VST_OPAQUE_PRESET_MAGIC) (%08x)", int(prog->fxMagic), int(BE_TO_CPU(chunkPresetMagic)));
                return STATUS_UNSUPPORTED_FORMAT;
            }

            // Validate fxID
            if (prog->fxID != BE_TO_CPU(VstInt32(pEffect->uniqueID)))
            {
                lsp_warn("prog->fxID (%08x) != BE_DATA(VstInt32(pEffect->uniqueID)) (%08x)", int(prog->fxID), int(BE_TO_CPU(VstInt32(pEffect->uniqueID))));
                return STATUS_UNSUPPORTED_FORMAT;
            }

            return STATUS_OK;
        }

        void Wrapper::deserialize_state(const void *data, size_t size)
        {
            // Set state manage barrier
            bStateManage = true;
            lsp_finally { bStateManage = false; };

            const fxBank *bank          = static_cast<const fxBank *>(data);
            const fxProgram *prog       = static_cast<const fxProgram *>(data);
            const uint8_t *head         = static_cast<const uint8_t *>(data);

            // Notify plugin that state is about to load
            pPlugin->before_state_load();

            // Do the state load
            status_t res;
            if ((res = check_vst_bank_header(bank, size)) == STATUS_OK)
            {
                lsp_warn("Found standard VST 2.x chunk header (bank)");
                dump_vst_bank(bank, (BE_TO_CPU(bank->byteSize) + 2 * sizeof(VstInt32)));

                // Check the version
                VstInt32 fxVersion = BE_TO_CPU(bank->fxVersion);
                if (fxVersion < VST_FX_VERSION_KVT_SUPPORT)
                    deserialize_v1(bank);       // Load V1 bank for legacy support
                else
                {
                    // Get size of chunk
                    size_t bytes                    = BE_TO_CPU(VstInt32(bank->byteSize));
                    if (bytes < offsetof(fxBank, content.data.chunk))
                    {
                        lsp_trace("byte_size (%d) < VST_STATE_BUFFER_SIZE (%d)", int(bytes), int(VST_STATE_BUFFER_SIZE));
                        return;
                    }

                    const uint8_t *tail = &head[bytes + VST_BANK_HDR_SKIP];
                    head               += offsetof(fxBank, content.data.chunk);
                    bytes               = BE_TO_CPU(bank->content.data.size);
                    if (size_t(tail - head) != bytes)
                    {
                        lsp_trace("Content size=0x%x does not match specified=0x%x", int(tail-head), int(bytes));
                        return;
                    }

                    deserialize_new_chunk_format(head, bytes);
                }
            }
            else if ((res = check_vst_program_header(prog, size)) == STATUS_OK)
            {
                // VST Programs do support only new chunk format
                lsp_warn("Found standard VST 2.x chunk header (program)");
                dump_vst_bank(prog, (BE_TO_CPU(prog->byteSize) + 2 * sizeof(VstInt32)));

                // Get size of chunk
                size_t bytes                    = BE_TO_CPU(VstInt32(prog->byteSize));
                if (bytes < offsetof(fxProgram, content.data.chunk))
                {
                    lsp_trace("byte_size (%d) < VST_STATE_BUFFER_SIZE (%d)", int(bytes), int(VST_STATE_BUFFER_SIZE));
                    return;
                }

                const uint8_t *tail = &head[bytes + VST_PROGRAM_HDR_SKIP];
                head               += offsetof(fxProgram, content.data.chunk);
                bytes               = BE_TO_CPU(prog->content.data.size);
                if (size_t(tail - head) != bytes)
                {
                    lsp_trace("Content size=0x%x does not match specified=0x%x", int(tail-head), int(bytes));
                    return;
                }

                deserialize_new_chunk_format(head, bytes);
            }
            else if (res == STATUS_NOT_FOUND)
            {
                // Do stuff considering that there is NO chunk headers, just raw data
                lsp_warn("No VST 2.x chunk header found, assuming the body is in valid state");
                dump_vst_bank(head, size);
                deserialize_new_chunk_format(head, size);
            }
            else
                return;

            // Call callback
            bUpdateSettings = true;
            lsp_trace("Plugin state has been loaded");

            // Notify the plugin that state has been loaded
            pPlugin->state_loaded();
        }

        void Wrapper::deserialize_new_chunk_format(const uint8_t *data, size_t bytes)
        {
            // Lookup extension header
            vst2::state_header_t hdr;
            ::bzero(&hdr, sizeof(hdr));
            if (bytes >= sizeof(vst2::state_header_t))
            {
                const vst2::state_header_t *src     = reinterpret_cast<const vst2::state_header_t *>(data);
                hdr.nMagic1     = BE_TO_CPU(src->nMagic1);
                hdr.nSize       = BE_TO_CPU(src->nSize);
                hdr.nVersion    = BE_TO_CPU(src->nVersion);
                hdr.nMagic2     = BE_TO_CPU(src->nMagic2);
            }

            // Analyze version
            if ((hdr.nMagic1 != LSP_VST_USER_MAGIC) || (hdr.nMagic2 != LSP_VST_USER_MAGIC))
            {
                lsp_debug("Performing V2 parameter deserialization (0x%x bytes)", int(bytes));
                deserialize_v2_v3(data, bytes);
            }
            else if (hdr.nVersion >= VST_FX_VERSION_JUCE_FIX)
            {
                lsp_debug("Performing V3 parameter deserialization");
                deserialize_v2_v3(&data[sizeof(hdr)], hdr.nSize);
            }
            else
                lsp_warn("Unsupported format, don't know how to deserialize chunk");
        }

        vst2::Port *Wrapper::find_by_id(const char *id)
        {
            ssize_t first=0, last = vSortedPorts.size() - 1;
            while (first <= last)
            {
                ssize_t mid             = (first + last) >> 1;
                vst2::Port *p           = vSortedPorts.uget(mid);
                int cmp                 = strcmp(id, p->metadata()->id);

                if (cmp < 0)
                    last                    = mid - 1;
                else if (cmp > 0)
                    first                   = mid + 1;
                else
                    return p;
            }

            return NULL;
        }

        void Wrapper::deserialize_v1(const fxBank *bank)
        {
            lsp_debug("Performing V1 parameter deserialization");

            // Get size of chunk
            size_t bytes                    = BE_TO_CPU(VstInt32(bank->byteSize));
            if (bytes < VST_STATE_BUFFER_SIZE)
            {
                lsp_trace("byte_size (%d) < VST_STATE_BUFFER_SIZE (%d)", int(bytes), int(VST_STATE_BUFFER_SIZE));
                return;
            }

            // Ready to de-serialize
            const vst2::state_t *state  = reinterpret_cast<const vst2::state_t *>(bank + 1);
            size_t params               = BE_TO_CPU(state->nItems);
            const uint8_t *ptr          = state->vData;
            const uint8_t *tail         = reinterpret_cast<const uint8_t *>(state) + bytes - sizeof(vst2::state_t);
            char param_id[MAX_PARAM_ID_BYTES];

            while ((params--) > 0)
            {
                // Deserialize port ID
                ssize_t delta           = vst2::deserialize_string(param_id, MAX_PARAM_ID_BYTES, ptr, tail - ptr);
                if (delta <= 0)
                {
                    lsp_error("Bank data corrupted");
                    return;
                }
                ptr                    += delta;

                // Find port
                lsp_trace("Deserializing port id=%s", param_id);
                vst2::Port *vp          = find_by_id(param_id);
                if (vp == NULL)
                {
                    lsp_error("Bank data corrupted: port id=%s not found", param_id);
                    return;
                }

                // Deserialize port data
                delta                   = vp->deserialize_v1(ptr, tail - ptr);
                if (delta <= 0)
                {
                    lsp_error("bank data corrupted, could not deserialize port id=%s", param_id);
                    return;
                }
                ptr                    += delta;
            }
        }

        void Wrapper::deserialize_v2_v3(const uint8_t *data, size_t bytes)
        {
            const uint8_t *head = data;
            const uint8_t *tail = &head[bytes];

            lsp_debug("Reading regular ports...");
            while (size_t(tail - head) >= sizeof(uint32_t))
            {
                // Read parameter length
                uint32_t len        = BE_TO_CPU(*(reinterpret_cast<const uint32_t *>(head))) + sizeof(uint32_t);
                if (len > size_t(tail - head))
                {
                    lsp_warn("Unexpected end of chunk while fetching parameter size");
                    return;
                }
                const uint8_t *next = &head[len];

                // Read name of port
                head               += sizeof(uint32_t);
                const char *name    = reinterpret_cast<const char *>(head);
                len                 = ::strnlen(name, next - head) + 1;
                if (len > size_t(next - head))
                {
                    lsp_warn("Unexpected end of chunk while fetching parameter name");
                    return;
                }
                if (name[0] == '/') // This is KVT port?
                {
                    head               -= sizeof(uint32_t); // Rollback head pointer
                    break;
                }
                head               += len;

                // Find port
                lsp_trace("Deserializing port id=%s", name);
                vst2::Port *vp          = find_by_id(name);
                if (vp == NULL)
                {
                    lsp_warn("Port id=%s not found, skipping", name);
                    head        = next;
                    continue;
                }

                // Deserialize port
                if (!vp->deserialize_v2(head, next - head))
                {
                    lsp_warn("Error deserializing port %s, skipping", name);
                    head        = next;
                    continue;
                }

                // Move to next parameter
                head        = next;
            }

            // Nothing to de-serialize more?
            if (head >= tail)
                return;

            // Deserialize KVT state
            lsp_debug("Reading KVT ports...");
            if (sKVTMutex.lock())
            {
                sKVT.clear();

                while (size_t(tail - head) >= sizeof(uint32_t))
                {
                    // Read parameter length
                    uint32_t len        = BE_TO_CPU(*(reinterpret_cast<const uint32_t *>(head))) + sizeof(uint32_t);
                    lsp_trace("Reading block: off=0x%x, size=%d", int(head - data), int(len));
                    if (len > size_t(tail - head))
                    {
                        lsp_warn("Unexpected end of chunk while fetching KVT parameter size");
                        break;
                    }
                    const uint8_t *next = &head[len];

                    // Read name of parameter
                    head               += sizeof(uint32_t);
                    const char *name    = reinterpret_cast<const char *>(head);
                    len                 = ::strnlen(name, next - head) + 1;
                    if (len > size_t(next - head))
                    {
                        lsp_warn("Unexpected end of chunk while fetching KVT parameter name");
                        break;
                    }
                    head               += len;

                    // Deserialize KVT parameter
                    core::kvt_param_t p;
                    p.type              = core::KVT_ANY;
                    uint8_t flags       = *(head++);
                    uint8_t type        = *(head++);

                    lsp_trace("Deserializing KVT parameter id=%s, type=0x%x", name, int(type));

                    switch (type)
                    {
                        case vst2::TYPE_INT32:
                            if ((next - head) != sizeof(int32_t))
                                break;
                            p.type      = core::KVT_INT32;
                            p.i32       = BE_TO_CPU(*(reinterpret_cast<const int32_t *>(head)));
                            head       += sizeof(int32_t);
                            break;

                        case vst2::TYPE_UINT32:
                            if ((next - head) != sizeof(uint32_t))
                                break;
                            p.type      = core::KVT_UINT32;
                            p.u32       = BE_TO_CPU(*(reinterpret_cast<const uint32_t *>(head)));
                            head       += sizeof(uint32_t);
                            break;

                        case vst2::TYPE_INT64:
                            if ((next - head) != sizeof(int64_t))
                                break;
                            p.type      = core::KVT_INT64;
                            p.i64       = BE_TO_CPU(*(reinterpret_cast<const int64_t *>(head)));
                            head       += sizeof(int64_t);
                            break;

                        case vst2::TYPE_UINT64:
                            if ((next - head) != sizeof(uint64_t))
                                break;
                            p.type      = core::KVT_UINT64;
                            p.u64       = BE_TO_CPU(*(reinterpret_cast<const uint64_t *>(head)));
                            head       += sizeof(uint64_t);
                            break;

                        case vst2::TYPE_FLOAT32:
                            if ((next - head) != sizeof(float))
                                break;
                            p.type      = core::KVT_FLOAT32;
                            p.f32       = BE_TO_CPU(*(reinterpret_cast<const float *>(head)));
                            head       += sizeof(float);
                            break;

                        case vst2::TYPE_FLOAT64:
                            if ((next - head) != sizeof(double))
                                break;
                            p.type      = core::KVT_FLOAT64;
                            p.f64       = BE_TO_CPU(*(reinterpret_cast<const double *>(head)));
                            head       += sizeof(double);
                            break;

                        case vst2::TYPE_STRING:
                            p.str       = reinterpret_cast<const char *>(head);
                            if (::strnlen(p.str, next-head) < size_t(next - head))
                                p.type      = core::KVT_STRING;
                            break;

                        case vst2::TYPE_BLOB:
                            p.blob.ctype    = reinterpret_cast<const char *>(head);
                            len             = ::strnlen(p.blob.ctype, next-head) + 1;
                            if (len > size_t(next - head))
                            {
                                lsp_trace("BLOB: clen=%d out of range %d", int(len), int(next-head));
                                break;
                            }

                            head           += len;
                            p.blob.size     = next - head;
                            p.blob.data     = (p.blob.size > 0) ? head : NULL;
                            p.type          = core::KVT_BLOB;
                            break;
                        default:
                            lsp_warn("Unknown KVT parameter type: %d ('%c') for id=%s", type, type, name);
                            break;
                    }

                    if (p.type != core::KVT_ANY)
                    {
                        size_t kflags = core::KVT_TX;
                        if (flags & vst2::FLAG_PRIVATE)
                            kflags     |= core::KVT_PRIVATE;

                        kvt_dump_parameter("Fetched parameter %s = ", &p, name);
                        sKVT.put(name, &p, kflags);
                    }

                    // Move to next parameter
                    head        = next;
                }

                sKVT.gc();
                sKVTMutex.unlock();
            }
        }

        core::KVTStorage *Wrapper::kvt_lock()
        {
            return (sKVTMutex.lock()) ? &sKVT : NULL;
        }

        core::KVTStorage *Wrapper::kvt_trylock()
        {
            return (sKVTMutex.try_lock()) ? &sKVT : NULL;
        }

        bool Wrapper::kvt_release()
        {
            return sKVTMutex.unlock();
        }

        void Wrapper::request_state_dump()
        {
            atomic_add(&nDumpReq, 1);
        }

        const meta::package_t *Wrapper::package() const
        {
            return pPackage;
        }

        meta::plugin_format_t Wrapper::plugin_format() const
        {
            return meta::PLUGIN_VST2;
        }

        core::SamplePlayer *Wrapper::sample_player()
        {
            return pSamplePlayer;
        }

        void Wrapper::request_settings_update()
        {
            bUpdateSettings     = true;
        }

        void Wrapper::state_changed()
        {
            if (bStateManage)
                return;

            if ((pMaster != NULL) && (pEffect != NULL))
                pMaster(pEffect, audioMasterUpdateDisplay, 0, 0, 0, 0);
        }

    } /* namespace vst2 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_WRAPPER_H_ */
