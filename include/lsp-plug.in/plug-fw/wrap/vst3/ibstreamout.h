/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 февр. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IBSTREAMOUT_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IBSTREAMOUT_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/io/IOutStream.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        // Standard Output Stream wrapper around the Steinberg::IBStream
        class IBStreamOut: public io::IOutStream
        {
            private:
                Steinberg::IBStream    *pOS;
                wssize_t                nPosition;

            public:
                explicit IBStreamOut(Steinberg::IBStream *out)
                {
                    pOS             = out;
                    nPosition       = 0;
                }

                IBStreamOut(const IOutStream &) = delete;
                IBStreamOut(IOutStream &&) = delete;
                virtual ~IBStreamOut() override
                {
                    IBStreamOut::close();
                }

                IBStreamOut & operator = (const IBStreamOut &) = delete;
                IBStreamOut & operator = (IBStreamOut &&) = delete;

            public: // io::IOutStream
                virtual wssize_t    position() override
                {
                    if (pOS == NULL)
                        return -set_error(STATUS_CLOSED);

                    set_error(STATUS_OK);
                    return nPosition;
                }

                virtual ssize_t     write(const void *buf, size_t count) override
                {
                    if (pOS == NULL)
                        return -set_error(STATUS_CLOSED);

                    Steinberg::int32 written = 0;
                    Steinberg::tresult res = pOS->write(const_cast<void *>(buf), count, &written);
                    if (res != Steinberg::kResultOk)
                        return -set_error(STATUS_IO_ERROR);

                    nPosition      += written;
                    set_error(STATUS_OK);
                    return written;
                }

                virtual status_t    write_byte(int v) override
                {
                    if (pOS == NULL)
                        return -set_error(STATUS_CLOSED);

                    uint8_t byte    = v;
                    Steinberg::int32 written = 0;
                    Steinberg::tresult res = pOS->write(&byte, 1, &written);
                    if (res != Steinberg::kResultOk)
                        return -set_error(STATUS_IO_ERROR);

                    nPosition      += written;
                    return set_error(STATUS_OK);
                }

                virtual wssize_t    seek(wsize_t position) override
                {
                    if (pOS == NULL)
                        return -set_error(STATUS_CLOSED);

                    Steinberg::int64 result = 0;
                    Steinberg::tresult res = pOS->seek(nPosition, Steinberg::IBStream::kIBSeekSet, &result);
                    if (res != Steinberg::kResultOk)
                        return -set_error(STATUS_IO_ERROR);

                    nPosition       = result;
                    set_error(STATUS_OK);
                    return nPosition;
                }

                virtual status_t    flush() override
                {
                    if (pOS == NULL)
                        return -set_error(STATUS_CLOSED);
                    return set_error(STATUS_OK);
                }

                virtual status_t    close() override
                {
                    if (pOS == NULL)
                        return set_error(STATUS_OK);

                    pOS     = NULL;
                    return set_error(STATUS_OK);
                }
        };

    } /* namespace vst3 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IBSTREAMOUT_H_ */
