/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_META_MANIFEST_H_
#define LSP_PLUG_IN_PLUG_FW_META_MANIFEST_H_

#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/io/IInSequence.h>
#include <lsp-plug.in/io/IInStream.h>

namespace lsp
{
    namespace meta
    {
        status_t load_manifest(meta::package_t **pkg, const char *path, const char *charset=NULL);
        status_t load_manifest(meta::package_t **pkg, const LSPString *path, const char *charset=NULL);
        status_t load_manifest(meta::package_t **pkg, const io::Path *path, const char *charset=NULL);
        status_t load_manifest(meta::package_t **pkg, io::IInStream *is, const char *charset=NULL);
        status_t load_manifest(meta::package_t **pkg, io::IInSequence *is);

        void free_manifest(meta::package_t *pkg);
    }
}


#endif /* LSP_PLUG_IN_PLUG_FW_META_MANIFEST_H_ */
