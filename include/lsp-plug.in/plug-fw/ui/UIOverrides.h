/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 сент. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_UIOVERRIDES_H_
#define LSP_PLUG_IN_PLUG_FW_UI_UIOVERRIDES_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace ui
    {
        /**
         * Attribute overrides (key=value pairs)
         */
        class UIOverrides
        {
            protected:
                typedef struct attribute_t
                {
                    LSPString   name;       // Name of attribute
                    LSPString   value;      // Associated value
                    ssize_t     refs;       // Number of references
                    ssize_t     depth;      // The actual attribute depth
                    ssize_t     vdepth;     // Visibilty depth
                } attribute_t;

                // Attribute list to override
                typedef struct attlist_t
                {
                    lltl::parray<attribute_t> items;
                    size_t depth;
                } attlist_t;

            protected:
                lltl::parray<attlist_t>     vStack;

            protected:
                static void                 drop_attlist(attlist_t *list);
                static bool                 attribute_present(const LSPString * const *atts, const LSPString *name);
                static void                 release_attribute(attribute_t *attr, size_t depth);

            public:
                explicit UIOverrides();
                ~UIOverrides();

            public:
                /**
                 * Create new state of overrides increasing depth
                 * @param depth depth increment
                 * @return status of operation
                 */
                status_t            push(size_t depth);

                /**
                 * Destroy the top state of overrides decreasing depth
                 * @return status of operation
                 */
                status_t            pop();

                /**
                 * Set new attribute (or replace previously existing)
                 * @param name attribute name
                 * @param value attribute value
                 * @param depth depth of the attribute
                 * @return status of operation
                 */
                status_t            set(const LSPString *name, const LSPString *value, ssize_t depth);

                /**
                 * Get attribute value by name
                 * @param name attribute name
                 * @return attribute value
                 */
                const LSPString    *get(const LSPString *name) const;

                /**
                 * Get attribute name by index
                 * @param index attribute index
                 * @return attribute name or NULL
                 */
                const LSPString    *name(size_t index) const;

                /**
                 * Get attribute value by index
                 * @param index attribute index
                 * @return attribute value or NULL
                 */
                const LSPString    *value(size_t index) const;

                /**
                 * Get number of attributes
                 * @return number of attributes
                 */
                size_t              count() const;

                /**
                 * Build list with overridden attributes
                 * @param dst destination list to store all attributes with NULL-terminating element
                 * @param atts source NULL-terminated list of key=value pairs
                 * @return status of operation
                 */
                status_t            build(lltl::parray<LSPString> *dst, const LSPString * const *atts);
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_UI_UIOVERRIDES_H_ */
