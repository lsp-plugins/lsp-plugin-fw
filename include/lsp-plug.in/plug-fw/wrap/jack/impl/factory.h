/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 6 авг. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/wrap/jack/factory.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace jack
    {
        Factory::Factory()
        {
            nReferences     = 1;
        }

        Factory::~Factory()
        {
        }

        size_t Factory::acquire()
        {
            return atomic_add(&nReferences, 1) + 1;
        }

        size_t Factory::release()
        {
            atomic_t ref_count = atomic_add(&nReferences, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        status_t Factory::create_plugin(plug::Module **module, const char *id)
        {
            // Lookup plugin identifier among all registered plugin factories
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Check plugin identifier
                    if (!::strcmp(meta->uid, id))
                    {
                        // Instantiate the plugin and return
                        plug::Module *m = f->create(meta);
                        if (m != NULL)
                        {
                            *module = m;
                            return STATUS_OK;
                        }

                        fprintf(stderr, "Plugin instantiation error: %s\n", id);
                        return STATUS_NO_MEM;
                    }
                }
            }

            // No plugin has been found
            return STATUS_NOT_FOUND;
        }

    #ifdef WITH_UI_FEATURE
        status_t Factory::create_ui(ui::Module **ui, const char *id)
        {
            // Lookup plugin identifier among all registered plugin factories
            for (ui::Factory *f = ui::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Check plugin identifier
                    if (!::strcmp(meta->uid, id))
                    {
                        // Instantiate the plugin UI and return
                        ui::Module *m = f->create(meta);
                        if (m != NULL)
                        {
                            *ui = m;
                            return STATUS_OK;
                        }

                        fprintf(stderr, "Plugin UI instantiation error: %s\n", id);
                        return STATUS_NO_MEM;
                    }
                }
            }

            // No plugin UI has been found
            return STATUS_NOT_FOUND;
        }
    #endif /* WITH_UI_FEATURE */

        core::Catalog *Factory::acquire_catalog()
        {
            return sCatalogManager.acquire();
        }

        void Factory::release_catalog(core::Catalog *catalog)
        {
            sCatalogManager.release(catalog);
        }
    } /* namespace jack */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_FACTORY_H_ */
