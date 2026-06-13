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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_DUMMY_IMPL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_DUMMY_IMPL_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/audio/iface/builtin.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/dummy/backend.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/dummy/factory.h>

#include <stdlib.h>

namespace lsp
{
    namespace audio
    {
        namespace dummy
        {
            const audio::backend_metadata_t factory_t::sMetadata[] =
            {
                {
                    "dummy",
                    "Dummy Audio Backend",
                    "dummy",
                    0
                }
            };

            const audio::backend_metadata_t *factory_t::metadata(audio::factory_t *self, size_t id)
            {
                const size_t count = sizeof(sMetadata) / sizeof(audio::backend_metadata_t);
                return (id < count) ? &sMetadata[id] : NULL;
            }

            audio::backend_t *factory_t::create(audio::factory_t *self, size_t id)
            {
                if (id == 0)
                {
                    dummy::backend_t * const res = static_cast<dummy::backend_t *>(::malloc(sizeof(dummy::backend_t)));
                    if (res != NULL)
                        res->construct();
                    return res;
                }
                return NULL;
            }

            factory_t::factory_t()
            {
                #define AUDIO_DUMMY_FACTORY_EXP(func)   audio::factory_t::func   = dummy::factory_t::func;
                AUDIO_DUMMY_FACTORY_EXP(create);
                AUDIO_DUMMY_FACTORY_EXP(metadata);
                #undef AUDIO_DUMMY_FACTORY_EXP
            }

            factory_t::~factory_t()
            {
            }

            // Builtin factory binding
            static factory_t   factory;
            LSP_AUDIO_BUILTIN_FACTORY(builtin_dummy_factory, &lsp::audio::dummy::factory);
        } /* namespace dummy */
    } /* namespace audio */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_DUMMY_IMPL_FACTORY_H_ */
