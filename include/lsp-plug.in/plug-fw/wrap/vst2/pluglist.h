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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_PLUGLIST_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_PLUGLIST_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

#include <steinberg/vst2.h>

namespace lsp
{
    namespace vst2
    {
        class Factory;

        /**
         * Plugin list
         */
        class PlugList
        {
            private:
                vst2::Factory              *pFactory;
                const meta::plugin_t      **vPlugins;
                size_t                      nCount;
                size_t                      nIndex;

            public:
                PlugList(vst2::Factory * factory, const meta::plugin_t **list, size_t count);
                PlugList(const PlugList &) = delete;
                PlugList(const PlugList &&) = delete;
                ~PlugList();

                PlugList & operator = (const PlugList &) = delete;
                PlugList & operator = (PlugList &&) = delete;

            public:
                const meta::plugin_t   *get_next();
                const meta::plugin_t   *get(size_t index) const;
                const meta::plugin_t   *current() const;
                vst2::Factory          *factory() const;
                void                    rewind();
        };

    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_PLUGLIST_H_ */
