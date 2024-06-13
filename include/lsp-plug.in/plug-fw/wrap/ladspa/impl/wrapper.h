/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 янв. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/ladspa/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/ladspa/ports.h>

namespace lsp
{
    namespace ladspa
    {
        Wrapper::Wrapper(plug::Module *plugin, resource::ILoader *loader): IWrapper(plugin, loader)
        {
            pExecutor       = NULL;
            nLatencyID      = -1;
            pLatency        = NULL;
            bUpdateSettings = true;
            pPackage        = NULL;

            plug::position_t::init(&sNewPosition);
        }

        Wrapper::~Wrapper()
        {
            do_destroy();
        }

        status_t Wrapper::init(unsigned long sr)
        {
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
            lsp_trace("Creating ports");
            lltl::parray<plug::IPort> plugin_ports;
            const meta::plugin_t *m = pPlugin->metadata();
            for (const meta::port_t *port = m->ports; port->id != NULL; ++port)
                create_port(&plugin_ports, port);

            // Store the latency ID port
            nLatencyID              = vExtPorts.size();

            // Store sample rate
            sPosition.sampleRate    = sr;
            sNewPosition.sampleRate = sr;

            // Initialize plugin
            lsp_trace("Initializing plugin");
            pPlugin->init(this, plugin_ports.array());
            pPlugin->set_sample_rate(sr);
            bUpdateSettings = true;

            // Return success status
            return STATUS_OK;
        }

        void Wrapper::destroy()
        {
            do_destroy();
        }

        void Wrapper::do_destroy()
        {
            // Clear all ports
            for (size_t i=0; i < vAllPorts.size(); ++i)
            {
                lsp_trace("destroy port id=%s", vAllPorts[i]->metadata()->id);
                delete vAllPorts[i];
            }
            vAllPorts.flush();
            vAudioIn.flush();
            vAudioOut.flush();
            vParams.flush();
            vMeters.flush();
            vExtPorts.flush();

            // Delete plugin
            if (pPlugin != NULL)
            {
                pPlugin->destroy();
                delete pPlugin;
                pPlugin     = NULL;
            }

            // Destroy executor
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                delete pExecutor;
                pExecutor   = NULL;
            }

            // Destroy package
            meta::free_manifest(pPackage);
            pPackage        = NULL;
            nLatencyID      = -1;
            pLatency        = NULL;
        }

        ladspa::Port *Wrapper::create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port)
        {
            ladspa::Port *result = NULL;

            switch (port->role)
            {
                case meta::R_AUDIO_IN:
                {
                    ladspa::AudioPort *ap   = new ladspa::AudioPort(port);
                    vExtPorts.add(ap);
                    vAudioIn.add(ap);
                    plugin_ports->add(ap);
                    result = ap;
                    lsp_trace("audio_in id=%s, index=%d", port->id, int(vExtPorts.size() - 1));
                    break;
                }

                case meta::R_AUDIO_OUT:
                {
                    ladspa::AudioPort *ap   = new ladspa::AudioPort(port);
                    vExtPorts.add(ap);
                    vAudioOut.add(ap);
                    plugin_ports->add(ap);
                    result = ap;
                    lsp_trace("audio_out id=%s, index=%d", port->id, int(vExtPorts.size() - 1));
                    break;
                }
                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    ladspa::InputPort *pp = new ladspa::InputPort(port);
                    vExtPorts.add(pp);
                    vParams.add(pp);
                    plugin_ports->add(pp);
                    result = pp;
                    lsp_trace("parameter id=%s, index=%d", port->id, int(vExtPorts.size() - 1));
                    break;
                }
                case meta::R_METER:
                {
                    ladspa::OutputPort *mp = new ladspa::OutputPort(port);
                    vExtPorts.add(mp);
                    vMeters.add(mp);
                    plugin_ports->add(mp);
                    result = mp;
                    lsp_trace("meter id=%s, index=%d", port->id, int(vExtPorts.size() - 1));
                    break;
                }

                // Not supported by LADSPA, make it as stub ports
                case meta::R_PORT_SET: // TODO: implement recursive port creation?
                {
                    ladspa::InputPort *ps = new ladspa::InputPort(port);
                    plugin_ports->add(ps);
                    vParams.add(ps);
                    result = ps;
                    lsp_trace("port_set id=%s, index=%d", port->id);
                    break;
                }

                case meta::R_PATH:
                {
                    ladspa::PathPort *pp = new ladspa::PathPort(port);
                    plugin_ports->add(pp);
                    result = pp;
                    lsp_trace("path id=%s", port->id);
                    break;
                }
                case meta::R_STRING:
                {
                    ladspa::StringPort *sp = new ladspa::StringPort(port);
                    plugin_ports->add(sp);
                    result = sp;
                    lsp_trace("string id=%s", port->id);
                    break;
                }

                case meta::R_MESH:
                case meta::R_STREAM:
                case meta::R_FBUFFER:
                case meta::R_MIDI_IN:
                case meta::R_MIDI_OUT:
                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                default:
                {
                    result  = new ladspa::Port(port);
                    plugin_ports->add(result);
                    lsp_trace("stub id=%s, index=%d", port->id, int(vExtPorts.size() - 1));
                    break;
                }
            }

            // Add the created port to complete list of ports
            if (result != NULL)
                vAllPorts.add(result);

            return result;
        }

        inline void Wrapper::activate()
        {
            sPosition.frame     = 0;
            sNewPosition.frame  = 0;
            pPlugin->activate();
        }

        inline void Wrapper::connect(size_t id, void *data)
        {
            ladspa::Port *p     = vExtPorts[id];
            if (p != NULL)
                p->bind(data);
            else if (id == nLatencyID) // Bind latency pointer
                pLatency        = reinterpret_cast<LADSPA_Data *>(data);
        }

        inline void Wrapper::run(size_t samples)
        {
            // Emulate the behaviour of position
            if (pPlugin->set_position(&sNewPosition))
                bUpdateSettings = true;
            sPosition = sNewPosition;
//                lsp_trace("frame = %ld, tick = %f", long(sPosition.frame), float(sPosition.tick));

            // Cleanup meters
            for (size_t i=0, n=vMeters.size(); i<n; ++i)
            {
                // Get port
                ladspa::OutputPort *port = vMeters.uget(i);
                if (port != NULL)
                    port->clear();
            }

            // Process parameter changes
            for (size_t i=0, n=vParams.size(); i<n; ++i)
            {
                // Get port
                ladspa::InputPort *port = vParams.uget(i);
                if (port == NULL)
                    continue;

                // Pre-process data in port
                if (port->changed())
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

            // Call the main processing unit (split data buffers into chunks of maximum LADSPA_MAX_BLOCK_LENGTH size)
            for (size_t off=0; off < samples; )
            {
                size_t to_process = samples - off;
                if (to_process > LADSPA_MAX_BLOCK_LENGTH)
                    to_process = LADSPA_MAX_BLOCK_LENGTH;

                // Sanitize input data
                for (size_t i=0, n=vAudioIn.size(); i < n; ++i)
                {
                    ladspa::AudioPort *port = vAudioIn.uget(i);
                    if (port != NULL)
                        port->sanitize_before(off, to_process);
                }
                for (size_t i=0, n=vAudioOut.size(); i < n; ++i)
                {
                    ladspa::AudioPort *port = vAudioOut.uget(i);
                    if (port != NULL)
                        port->sanitize_before(off, to_process);
                }

                // Process samples
                pPlugin->process(to_process);

                // Sanitize output data
                for (size_t i=0, n=vAudioIn.size(); i < n; ++i)
                {
                    ladspa::AudioPort *port = vAudioIn.uget(i);
                    if (port != NULL)
                        port->sanitize_after(off, to_process);
                }
                for (size_t i=0, n=vAudioOut.size(); i < n; ++i)
                {
                    ladspa::AudioPort *port = vAudioOut.uget(i);
                    if (port != NULL)
                        port->sanitize_after(off, to_process);
                }

                off += to_process;
            }

            // Output meter values
            for (size_t i=0, n=vMeters.size(); i<n; ++i)
            {
                // Get port
                ladspa::OutputPort *port = vMeters.uget(i);
                if (port != NULL)
                    port->sync();
            }

            // Write latency
            if (pLatency != NULL)
                *pLatency       = pPlugin->latency();

            // Move the position
            size_t spb          = sNewPosition.sampleRate / sNewPosition.beatsPerMinute; // samples per beat
            sNewPosition.frame += samples;
            sNewPosition.tick   = ((sNewPosition.frame % spb) * sNewPosition.ticksPerBeat) / spb;
        }

        inline void Wrapper::deactivate()
        {
            pPlugin->deactivate();
        }

        ipc::IExecutor *Wrapper::executor()
        {
            if (pExecutor == NULL)
            {
                lsp_trace("Creating native executor service");
                pExecutor       = new ipc::NativeExecutor();
            }
            return pExecutor;
        }

        const meta::package_t *Wrapper::package() const
        {
            return pPackage;
        }

        void Wrapper::request_settings_update()
        {
            bUpdateSettings     = true;
        }

        meta::plugin_format_t Wrapper::plugin_format() const
        {
            return meta::PLUGIN_LADSPA;
        }

    } /* namespace ladspa */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_IMPL_WRAPPER_H_ */
