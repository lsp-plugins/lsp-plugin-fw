/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
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
 * GNU Lesser General Public License for more des.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugin-fw. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/singletone.h>
#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/dsp/dsp.h>

#include <lsp-plug.in/plug-fw/wrap/gstreamer/defs.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/executor.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/factory.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/ports.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/impl/executor.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/impl/factory.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/impl/wrapper.h>

namespace lsp
{
    namespace gst
    {
        IFactory::~IFactory()
        {
        }

        IWrapper::~IWrapper()
        {
        }

        static gst::Factory *gstreamer_factory = NULL;
        static lsp::singletone_t library;

        void release_factory()
        {
            safe_release(gstreamer_factory);
        }

        static StaticFinalizer finalizer(release_factory);

        gst::Factory *get_factory()
        {
            // Check that library is already initialized
            if (library.initialized())
                return gstreamer_factory;

        #ifndef LSP_IDE_DEBUG
            IF_DEBUG( lsp::debug::redirect("lsp-gstreamer-lib.log"); );
        #endif /* LSP_IDE_DEBUG */

            // Initialize DSP
            lsp::dsp::init();

            // Allocate factory
            gst::Factory *factory = new gst::Factory();
            if (factory == NULL)
                return NULL;
            lsp_finally {
                safe_release(factory);
            };

            // Initialize factory
            status_t res = factory->init();
            if (res != STATUS_OK)
                return NULL;

            // Commit the loaded factory
            lsp_singletone_init(library) {
                gstreamer_factory = safe_acquire(factory);
            };

            return gstreamer_factory;
        }

    } /* namespace gst */
} /* namespace lsp */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    LSP_EXPORT_MODIFIER
    lsp::gst::IFactory *GSTREAMER_FACTORY_FUNCTION()
    {
        return lsp::gst::get_factory();
    }

#ifdef __cplusplus
}
#endif /* __cplusplus */



