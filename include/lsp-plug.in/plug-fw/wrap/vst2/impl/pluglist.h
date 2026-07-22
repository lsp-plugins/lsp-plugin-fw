/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 июл. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_PLUGLIST_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_PLUGLIST_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/pluglist.h>

namespace lsp
{
    namespace vst2
    {
        PlugList::PlugList(vst2::Factory *factory, const meta::plugin_t **list, size_t count)
        {
            pFactory        = factory;
            vPlugins        = list;
            nCount          = count;
            nIndex          = 0;
        }

        PlugList::~PlugList()
        {
            if (vPlugins != NULL)
            {
                free(vPlugins);
                vPlugins       = NULL;
            }
            nCount          = 0;
            nIndex          = 0;
        }

        const meta::plugin_t *PlugList::get_next()
        {
            return (nIndex < nCount) ? vPlugins[nIndex++] : 0;
        }

        const meta::plugin_t *PlugList::get(size_t index) const
        {
            return (index < nCount) ? vPlugins[index] : 0;
        }

        const meta::plugin_t *PlugList::current() const
        {
            if (nIndex <= 0)
                return NULL;
            return get(nIndex - 1);
        }

        void PlugList::rewind()
        {
            nIndex          = 0;
        }

        vst2::Factory *PlugList::factory() const
        {
            return pFactory;
        }

    } /* namespace vst2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_PLUGLIST_H_ */
