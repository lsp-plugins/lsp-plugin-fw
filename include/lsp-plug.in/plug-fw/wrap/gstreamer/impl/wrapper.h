/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 25 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/wrapper.h>

namespace lsp
{
    namespace gst
    {
        Wrapper::~Wrapper()
        {
            // TODO
        }

        void Wrapper::destroy()
        {
            // TODO
        }

        void Wrapper::setup(const GstAudioInfo * info)
        {
            // TODO
        }

        void Wrapper::set_property(guint prop_id, const GValue *value, GParamSpec *pspec)
        {
            // TODO
        }

        void Wrapper::get_property(guint prop_id, GValue * value, GParamSpec * pspec)
        {
            // TODO
        }

        void Wrapper::process(guint8 *out, const guint8 *in, size_t out_size, size_t in_size)
        {
            // TODO
        }

    } /* namespace gst */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_WRAPPER_H_ */


