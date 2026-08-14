/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 15 апр. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_DUMMY_IMPL_BACKEND_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_DUMMY_IMPL_BACKEND_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/finally.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/expr/Tokenizer.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/dummy/backend.h>
#include <lsp-plug.in/stdlib/string.h>

#include <stdlib.h>

namespace lsp
{
    namespace audio
    {
        namespace dummy
        {
            static constexpr uint32_t PORT_TYPE_FREE        = 0xffffffff;
            static constexpr uint32_t PORT_MASK_ALL         = PORT_DIR_MASK | PORT_TYPE_MASK;

            static inline dummy::backend_t *cast(audio::backend_t *self)
            {
                return static_cast<dummy::backend_t *>(self);
            }

            static inline dummy::backend_t *cast(void *self)
            {
                return static_cast<dummy::backend_t *>(self);
            }

            backend_t::backend_t()
            {
                construct();
            }

            void backend_t::construct()
            {
                pThread                         = NULL;
                pUserData                       = NULL;
                pCallbacks                      = NULL;

                io_parameters_t * const ip      = &sIOParams;
                ip->sample_rate                 = DEFAULT_BACKEND_SAMPLE_RATE;
                ip->buffer_size                 = DEFAULT_BACKEND_BUFFER_SIZE;
                ip->max_buffer_size             = DEFAULT_BACKEND_BUFFER_SIZE;

                io_position_t * const npos      = &sIOPosition;
                npos->frame                     = 0;
                npos->bar                       = 0;
                npos->beat                      = 0;
                npos->tick                      = 0;
                npos->speed                     = 1.0f;
                npos->numerator                 = 4.0f;
                npos->denominator               = 4.0f;
                npos->beats_per_minute          = 120.0f;
                npos->beats_per_minute_change   = 0.0f;
                npos->ticks_per_beat            = 4096.0f;

                vPorts                          = NULL;
                nFirst                          = 0;
                nCapacity                       = 0;

                // Export virtual table
                #define AUDIO_DUMMY_BACKEND_EXP(func)       audio::backend_t::func = dummy::backend_t::func;

                AUDIO_DUMMY_BACKEND_EXP(connect);
                AUDIO_DUMMY_BACKEND_EXP(set_latency);
                AUDIO_DUMMY_BACKEND_EXP(disconnect);
                AUDIO_DUMMY_BACKEND_EXP(destroy);

                AUDIO_DUMMY_BACKEND_EXP(register_port);
                AUDIO_DUMMY_BACKEND_EXP(unregister_port);
                AUDIO_DUMMY_BACKEND_EXP(port_system_name);

                AUDIO_DUMMY_BACKEND_EXP(connect_ports);
                AUDIO_DUMMY_BACKEND_EXP(disconnect_ports);

                AUDIO_DUMMY_BACKEND_EXP(audio_buffers_count);
                AUDIO_DUMMY_BACKEND_EXP(get_audio_buffer);

                AUDIO_DUMMY_BACKEND_EXP(read_midi_event);
                AUDIO_DUMMY_BACKEND_EXP(write_midi_event);

                #undef AUDIO_DUMMY_BACKEND_EXP
            }

            status_t backend_t::thread_main(void *self)
            {
                backend_t * const back = cast(self);

                dsp::init();
                dsp::context_t ctx;
                dsp::start(&ctx);
                lsp_finally {
                    dsp::finish(&ctx);
                };

                const size_t buffer_size    = back->sIOParams.buffer_size;
                const float period          = float(buffer_size * 1000.0f) / float(back->sIOParams.sample_rate);
                float delta                 = 0.0f;
                system::time_millis_t start = system::get_time_millis();

                while (!ipc::Thread::is_cancelled())
                {
                    // Call processing function
                    if ((back->pCallbacks) && (back->pCallbacks->on_process))
                    {
                        // Cleanup buffers
                        dsp::fill_zero(back->pInBuffer, buffer_size);
                        dsp::fill_zero(back->pOutBuffer, buffer_size);

                        back->pCallbacks->on_process(
                            back->pUserData,
                            &back->sIOPosition,
                            buffer_size);
                    }

                    // Sleep for a while
                    const system::time_millis_t current     = system::get_time_millis();
                    const float delay                       = period + delta - (current - start);
                    if (delay > 0.0f)
                    {
                        const system::time_millis_t sleep       = system::time_millis_t(delay);
                        delta                                   = delay - float(sleep);
                        if (sleep > 0)
                            ipc::Thread::sleep(sleep);
                    }

                    // Update start time point of the period
                    start                                   = current;
                }

                return STATUS_OK;
            }

            status_t backend_t::parse_uint(size_t & dst, const LSPString & name, const LSPString & value)
            {
                io::InStringSequence is(&value);
                expr::Tokenizer t(&is);

                switch (t.get_token(expr::TF_GET))
                {
                    case expr::TT_IVALUE:
                        dst = uint32_t(t.int_value());
                        break;
                    default:
                        fprintf(stderr, "Bad value for the '%s' parameter: %s\n", name.get_utf8(), value.get_native());
                        return STATUS_INVALID_VALUE;
                }

                if (t.get_token(expr::TF_GET) != expr::TT_EOF)
                {
                    fprintf(stderr, "Bad value for the '%s' parameter: %s\n", name.get_utf8(), value.get_native());
                    return STATUS_INVALID_VALUE;
                }

                return STATUS_OK;
            }

            status_t backend_t::parse_connection_param(const LSPString & name, const LSPString & value)
            {
                status_t res = STATUS_OK;
                if (name.equals_ascii("sample_rate"))
                {
                    res = parse_uint(sIOParams.sample_rate, name, value);
                    if ((sIOParams.sample_rate < 8000) || (sIOParams.sample_rate > 384000))
                    {
                        fprintf(stderr, "Sample rate %d out of range [8000, 384000] Hz\n", int(sIOParams.sample_rate));
                        return STATUS_INVALID_VALUE;
                    }
                }
                else if (name.equals_ascii("buffer_size"))
                {
                    res = parse_uint(sIOParams.buffer_size, name, value);
                    if ((sIOParams.buffer_size < 32) || (sIOParams.buffer_size > 16384))
                    {
                        fprintf(stderr, "Buffer size %d out of range [32, 16384] samples\n", int(sIOParams.buffer_size));
                        return STATUS_INVALID_VALUE;
                    }

                    sIOParams.max_buffer_size   = sIOParams.buffer_size;
                }
                return res;
            }

            status_t backend_t::parse_connection_params(const char *params)
            {
                io_parameters_t * const ip      = &sIOParams;
                ip->sample_rate                 = DEFAULT_BACKEND_SAMPLE_RATE;
                ip->buffer_size                 = DEFAULT_BACKEND_BUFFER_SIZE;
                ip->max_buffer_size             = DEFAULT_BACKEND_BUFFER_SIZE;

                if (params == NULL)
                    return STATUS_OK;

                status_t res;
                LSPString text, name, value, *str;
                if (!text.set_utf8(params))
                    return STATUS_NO_MEM;

                str = &name;
                lsp_wchar_t curr = 0;
                size_t count = 0;
                for (size_t i=0, n=text.length(); i<n; ++i)
                {
                    lsp_wchar_t prev    = curr;
                    curr                = text.char_at(i);
                    if (prev == '\\')
                    {
                        switch (curr)
                        {
                            case '\\':
                            case '/':
                            case ' ':
                            case '=':
                            case ',':
                                break;
                            case 'r': curr = '\r'; break;
                            case 'n': curr = '\n'; break;
                            case 't': curr = '\t'; break;
                            case 'v': curr = '\v'; break;
                            default:
                                if (!str->append(prev))
                                    return STATUS_NO_MEM;
                                break;
                        }
                        if (!str->append(curr))
                            return STATUS_NO_MEM;
                        ++count;
                        curr    = 0;
                    }
                    else
                    {
                        switch (curr)
                        {
                            case '\\':
                                break;
                            case '=':
                                ++count;
                                if (str == &value)
                                {
                                    if (!str->append(curr))
                                        return STATUS_NO_MEM;
                                }
                                else // dst == &key
                                    str = &value;
                                break;
                            case ',':
                                // Add and cleanup strings
                                if ((res = parse_connection_param(name, value)) != STATUS_OK)
                                    return res;
                                str     = &name;
                                name.clear();
                                value.clear();
                                count = 0;
                                break;
                            default:
                                if (!str->append(curr))
                                    return STATUS_NO_MEM;
                                ++count;
                                break;
                        }
                    }
                }

                // Parse last parameter if present
                if (count > 0)
                {
                    if ((res = parse_connection_param(name, value)) != STATUS_OK)
                        return res;
                }

                return STATUS_OK;
            }

            status_t backend_t::connect(
                audio::backend_t *self,
                const connection_params_t *params,
                const callbacks_t *callbacks,
                void *user_data)
            {
                backend_t * const back = cast(self);

                // Parse connection parameters
                const io_parameters_t * const ip = &back->sIOParams;
                status_t res            = back->parse_connection_params(params->url);
                if (res != STATUS_OK)
                    return res;

                // Check that backend is disconnected
                if (back->pThread != NULL)
                    return STATUS_BAD_STATE;

                // Initialize buffers
                uint8_t *data           = NULL;

                float * buffers         = alloc_aligned<float>(data, sizeof(float) * ip->buffer_size * 2, 0x40);
                back->pInBuffer         = advance_ptr<float>(buffers, ip->buffer_size);
                back->pOutBuffer        = advance_ptr<float>(buffers, ip->buffer_size);
                if (buffers == NULL)
                    return STATUS_NO_MEM;
                lsp_finally {
                    if (data != NULL)
                    {
                        back->pInBuffer         = NULL;
                        back->pOutBuffer        = NULL;
                        free(data);
                    }
                };

                // Create main thread
                ipc::Thread *thread = new ipc::Thread(thread_main, self);
                if (thread == NULL)
                {
                    lsp_warn("Could not create backend thread");
                    return STATUS_NO_MEM;
                }
                lsp_finally {
                    if (thread!= NULL)
                        delete thread;
                };

                // Commit state
                back->pThread       = thread;
                back->pUserData     = user_data;
                back->pCallbacks    = callbacks;

                lsp_finally {
                    if (thread != NULL)
                    {
                        back->pThread       = NULL;
                        back->pUserData     = NULL;
                        back->pCallbacks    = NULL;
                    }
                };

                // Issue connected callback
                res = ((callbacks) && (callbacks->on_connected)) ?
                    callbacks->on_connected(user_data, ip) :
                    STATUS_OK;
                lsp_finally {
                    if ((thread != NULL) && (callbacks) && (callbacks->on_connection_lost))
                        callbacks->on_connection_lost(user_data);
                };
                if (res != STATUS_OK)
                    return res;

                // Start the thread
                res = thread->start();
                if (res != STATUS_OK)
                {
                    lsp_warn("Could not start backend thread: error=%d", int(res));
                    return STATUS_NO_MEM;
                }
                lsp_finally {
                    if (thread!= NULL)
                    {
                        thread->cancel();
                        thread->join();
                    }
                };

                // Issue activated callback
                res = ((callbacks) && (callbacks->on_activated)) ?
                    callbacks->on_activated(user_data) :
                    STATUS_OK;
                if (res != STATUS_OK)
                    return res;

                // Do not close client on successful connection
                thread              = NULL;
                back->pData         = release_ptr(data);

                return STATUS_OK;
            }

            status_t backend_t::set_latency(audio::backend_t *self, uint32_t latency)
            {
                return STATUS_OK;
            }

            status_t backend_t::disconnect(audio::backend_t *self)
            {
                backend_t * const back          = cast(self);
                ipc::Thread * const thread      = back->pThread;
                if (thread == NULL)
                    return STATUS_BAD_STATE;

                // Deactivate thread
                thread->cancel();
                thread->join();

                const callbacks_t * const cb = back->pCallbacks;
                status_t res = ((cb) && (cb->on_deactivated)) ?
                    cb->on_deactivated(back->pUserData) : STATUS_OK;

                // Drop thread
                delete thread;
                back->pThread                   = NULL;
                if (back->pData != NULL)
                {
                    free_aligned(back->pData);

                    back->pInBuffer                 = NULL;
                    back->pOutBuffer                = NULL;
                    back->pData                     = NULL;
                }

                if ((cb) && (cb->on_disconnected))
                    cb->on_disconnected(back->pUserData);

                // Cleanup I/O parameters
                io_parameters_t * const ip      = &back->sIOParams;
                ip->sample_rate                 = DEFAULT_BACKEND_SAMPLE_RATE;
                ip->buffer_size                 = DEFAULT_BACKEND_BUFFER_SIZE;
                ip->max_buffer_size             = DEFAULT_BACKEND_BUFFER_SIZE;

                // Cleanup I/O position
                io_position_t * const npos      = &back->sIOPosition;
                npos->frame                     = 0;
                npos->bar                       = 0;
                npos->beat                      = 0;
                npos->tick                      = 0;
                npos->speed                     = 1.0f;
                npos->numerator                 = 4.0f;
                npos->denominator               = 4.0f;
                npos->beats_per_minute          = 120.0f;
                npos->beats_per_minute_change   = 0.0f;
                npos->ticks_per_beat            = 4096.0f;

                return res;
            }

            void backend_t::destroy(audio::backend_t *self)
            {
                backend_t * const back          = cast(self);

                // Issue disconnect and free allocated memory
                disconnect(self);

                // Free allocated memory for ports
                back->nFirst                = 0;
                back->nCapacity             = 0;
                if (back->vPorts != NULL)
                {
                    free(back->vPorts);
                    back->vPorts                = NULL;
                }

                // Deallocate memory
                free(self);
            }

            backend_t::port_t *backend_t::alloc_port(const char *id, uint32_t flags)
            {
                uint32_t first      = nFirst;
                uint32_t capacity   = nCapacity;

                // Check that memory should be re-allocated
                if (first >= capacity)
                {
                    const size_t new_cap    = lsp_max((capacity << 1), 4u);
                    port_t * const items    = static_cast<port_t *>(realloc(vPorts, sizeof(port_t) * new_cap));
                    if (!items)
                        return NULL;

                    for (size_t i=capacity; i<new_cap; ++i)
                    {
                        port_t * const port     = &items[i];
                        port->nType             = PORT_TYPE_FREE;
                        port->sID[0]            = '\0';
                    }

                    capacity                = new_cap;
                    vPorts                  = items;
                    nCapacity               = capacity;
                }

                // Find unused port in list
                for ( ; first < capacity; ++first)
                {
                    port_t * const port     = &vPorts[first];
                    if (port->nType == PORT_TYPE_FREE)
                    {
                        port->nType             = flags & PORT_MASK_ALL;
                        strncpy(port->sID, id, MAX_PORT_ID_BYTES);
                        port->sID[MAX_PORT_ID_BYTES-1]  = '\0';

                        nFirst                  = first + 1;
                        return port;
                    }
                }

                nFirst              = first;

                return NULL;
            }

            void backend_t::free_port(port_t *port)
            {
                if (port == NULL)
                    return;

                port->nType         = PORT_TYPE_FREE;
                nFirst              = lsp_min(nFirst, port_id_t(port - vPorts));
            }

            port_id_t backend_t::register_port(audio::backend_t *self, const char *id, uint32_t flags)
            {
                backend_t * const back  = cast(self);

                // Check arguments
                if (strlen(id) >= MAX_PORT_ID_BYTES)
                    return -STATUS_TOO_BIG;

                // Add port
                port_t *port = back->alloc_port(id, flags);
                if (port == NULL)
                    return STATUS_NO_MEM;

                return port_id_t(release_ptr(port) - back->vPorts);
            }

            status_t backend_t::unregister_port(audio::backend_t *self, port_id_t port_id)
            {
                backend_t * const back  = cast(self);

                if ((port_id < 0) || (port_id >= back->nCapacity))
                    return STATUS_INVALID_VALUE;

                port_t * const port = &back->vPorts[port_id];
                if (port->nType == PORT_TYPE_FREE)
                    return STATUS_INVALID_VALUE;

                back->free_port(port);

                return STATUS_OK;
            }

            const char *backend_t::port_system_name(audio::backend_t *self, port_id_t port_id)
            {
                backend_t * const back  = cast(self);

                if ((port_id < 0) || (port_id >= back->nCapacity))
                    return NULL;

                port_t * const port = &back->vPorts[port_id];
                if (port->nType == PORT_TYPE_FREE)
                    return NULL;

                return port->sID;
            }

            status_t backend_t::connect_ports(audio::backend_t *self, const char *source, const char *destination)
            {
                return STATUS_NOT_FOUND;
            }

            status_t backend_t::disconnect_ports(audio::backend_t *self, const char *source, const char *destination)
            {
                return STATUS_NOT_BOUND;
            }

            size_t backend_t::audio_buffers_count(audio::backend_t *self, port_id_t port_id)
            {
                backend_t * const back  = cast(self);
                if (back->pThread == NULL)
                    return 0;
                if ((port_id < 0) || (port_id >= back->nCapacity))
                    return 0;

                port_t * const port = &back->vPorts[port_id];
                if ((port->nType == PORT_TYPE_FREE) ||
                    ((port->nType & PORT_TYPE_MASK) != PORT_TYPE_AUDIO))
                    return 0;
                return 1;
            }

            float *backend_t::get_audio_buffer(audio::backend_t *self, port_id_t port_id, size_t index)
            {
                backend_t * const back  = cast(self);
                if ((port_id < 0) || (port_id >= back->nCapacity))
                    return NULL;

                port_t * const port = &back->vPorts[port_id];
                if (port->nType == PORT_AUDIO_IN)
                    return back->pInBuffer;
                else if (port->nType == PORT_AUDIO_OUT)
                    return back->pOutBuffer;

                return NULL;
            }

            status_t backend_t::read_midi_event(audio::backend_t *self, port_id_t port_id, midi_event_t *event, uint32_t *index)
            {
                if ((event == NULL) || (index == NULL))
                    return STATUS_BAD_ARGUMENTS;

                backend_t * const back  = cast(self);
                if (back->pThread == NULL)
                    return STATUS_BAD_STATE;
                if ((port_id < 0) || (port_id >= back->nCapacity))
                    return STATUS_INVALID_VALUE;

                port_t * const port = &back->vPorts[port_id];
                if (port->nType != PORT_MIDI_IN)
                    return STATUS_BAD_FORMAT;

                return (*index == 0) ? STATUS_NO_DATA : STATUS_OVERFLOW;
            }

            uint8_t *backend_t::write_midi_event(audio::backend_t *self, port_id_t port_id, uint32_t timestamp, uint32_t size)
            {
               return NULL;
            }

        } /* namespace dummy */
    } /* namespace audio */
} /* namespace lsp */






#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_DUMMY_IMPL_BACKEND_H_ */
