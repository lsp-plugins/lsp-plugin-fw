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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/factory.h>

namespace lsp
{
    namespace gst
    {
        Factory::Factory()
        {
            pLoader         = NULL;
            pPackage        = NULL;
            nReferences     = 1;
            nRefExecutor    = 0;
            pExecutor       = NULL;
        }

        Factory::~Factory()
        {
            destroy();
        }

        void Factory::destroy()
        {
            // Shutdown executor
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                delete pExecutor;
            }

            // Forget the package
            if (pPackage != NULL)
            {
                meta::free_manifest(pPackage);
                pPackage    = NULL;
            }

            // Release resource loader
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader     = NULL;
            }
        }

        status_t Factory::init()
        {
            // Create resource loader
            pLoader             = core::create_resource_loader();
            if (pLoader == NULL)
            {
                lsp_error("No resource loader available");
                return STATUS_BAD_STATE;
            }

            // Obtain the manifest
            lsp_trace("Obtaining manifest...");

            status_t res;
            meta::package_t *manifest = NULL;
            io::IInStream *is = pLoader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is != NULL)
            {
                lsp_finally {
                    is->close();
                    delete is;
                };

                if ((res = meta::load_manifest(&manifest, is)) != STATUS_OK)
                {
                    lsp_warn("Error loading manifest file, error=%d", int(res));
                    manifest = NULL;
                }
            }
            if (manifest == NULL)
            {
                lsp_trace("No manifest file found");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                meta::free_manifest(manifest);
            };

            // Remember the package
            pPackage    = release_ptr(manifest);

            return STATUS_OK;
        }

        atomic_t Factory::acquire()
        {
            return atomic_add(&nReferences, 1) + 1;
        }

        atomic_t Factory::release()
        {
            const atomic_t ref_count = atomic_add(&nReferences, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        ipc::IExecutor *Factory::acquire_executor()
        {
            lsp_trace("this=%p", this);

            if (!sMutex.lock())
                return NULL;
            lsp_finally { sMutex.unlock(); };

            // Try to perform quick access
            if (pExecutor != NULL)
            {
                ++nRefExecutor;
                return pExecutor;
            }

            // Create executor
            ipc::NativeExecutor *executor = new ipc::NativeExecutor();
            if (executor == NULL)
                return NULL;
            lsp_trace("Allocated executor=%p", executor);

            // Launch executor
            status_t res = executor->start();
            if (res != STATUS_OK)
            {
                lsp_trace("Failed to start executor=%p, code=%d", executor, int(res));
                delete executor;
                return NULL;
            }

            // Update status
            ++nRefExecutor;
            return pExecutor = executor;
        }

        void Factory::release_executor()
        {
            lsp_trace("this=%p", this);

            if (!sMutex.lock())
                return;
            lsp_finally { sMutex.unlock(); };

            if ((--nRefExecutor) > 0)
                return;
            if (pExecutor == NULL)
                return;

            lsp_trace("Destroying executor pExecutor=%p", pExecutor);
            pExecutor->shutdown();
            delete pExecutor;
            pExecutor   = NULL;
        }

        const meta::package_t *Factory::package() const
        {
            return pPackage;
        }

        void Factory::init_class(GstElementClass *element, const char *plugin_id)
        {
            // TODO
        }

        Wrapper *Factory::instantiate(const char *plugin_id)
        {
            gst::Wrapper *wrapper = NULL;

            // TODO

            lsp::status_t res   = wrapper->init();
            if (res != lsp::STATUS_OK)
            {
                delete wrapper;
                return NULL;
            }

            return wrapper;
        }

    } /* namespace gst */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_FACTORY_H_ */
