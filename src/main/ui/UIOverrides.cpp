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

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ui
    {
        UIOverrides::UIOverrides()
        {
        }

        UIOverrides::~UIOverrides()
        {
            // Drop all attributes
            for (ssize_t i=vStack.size() - 1; i >= 0; --i)
            {
                attlist_t *list = vStack.uget(i);
                drop_attlist(list);
            }
            vStack.flush();
        }

        void UIOverrides::release_attribute(attribute_t *attr, size_t depth)
        {
            if (attr == NULL)
                return;

            attr->depth -= depth;
            if ((-- attr->refs) <= 0)
                delete attr;
        }

        void UIOverrides::drop_attlist(attlist_t *list)
        {
            lltl::parray<attribute_t> *items = &list->items;
            for (size_t i=0, n=items->size(); i<n; ++i)
                release_attribute(items->uget(i), list->depth);
            items->flush();
            delete list;
        }

        status_t UIOverrides::push(size_t depth)
        {
            attlist_t *list = new attlist_t();
            if (list == NULL)
                return STATUS_NO_MEM;
            list->depth     = depth;

            // We need to make a copy of current list
            attlist_t *src = vStack.last();
            if (src != NULL)
            {
                size_t num_items = src->items.size();
                list->items.reserve(num_items);

                for (size_t i=0; i<num_items; ++i)
                {
                    attribute_t *satt = list->items.uget(i);

                    // Skip attributes with limited visibility depth
                    if ((satt->vdepth >= 0) && (ssize_t(satt->depth + depth) < satt->vdepth))
                        continue;

                    // Try to add item to the list
                    if (!list->items.add(satt))
                    {
                        drop_attlist(list);
                        return STATUS_NO_MEM;
                    }

                    // Increate the reference counter
                    satt->depth    += depth;
                    ++satt->refs;
                }
            }

            // Now push new list to stack
            if (!vStack.push(list))
            {
                drop_attlist(list);
                return STATUS_NO_MEM;
            }

            return STATUS_OK;
        }

        status_t UIOverrides::pop()
        {
            attlist_t *list = vStack.pop();
            if (list == NULL)
                return STATUS_UNDERFLOW;

            drop_attlist(list);
            return STATUS_OK;
        }

        status_t UIOverrides::set(const LSPString *name, const LSPString *value, ssize_t depth)
        {
            // Get attribute list to modify
            attlist_t *list = vStack.last();
            if (list == NULL)
                return STATUS_BAD_STATE;

            // Create attribute object
            attribute_t *att = new attribute_t();
            if (att == NULL)
                return STATUS_NO_MEM;
            if ((!att->name.set(name)) || (!att->value.set(value)))
            {
                delete att;
                return STATUS_NO_MEM;
            }
            att->refs   = 1;
            att->depth  = 0;
            att->vdepth = depth;

            // Replace existing attribute if it is present
            for (size_t i=0, n=list->items.size(); i<n; ++i)
            {
                attribute_t *curr = list->items.uget(i);
                if (curr == NULL)
                {
                    delete att;
                    return STATUS_CORRUPTED;
                }

                if (!curr->name.equals(&att->name))
                    continue;

                // Try to replace the attribute
                if (!list->items.set(i, att))
                {
                    delete att;
                    return STATUS_NO_MEM;
                }

                // Release overridden attribute
                release_attribute(curr, list->depth);

                // Attribute has been added
                return STATUS_OK;
            }

            // There is no attribute with such name present, add new
            if (!list->items.add(att))
            {
                delete att;
                return STATUS_NO_MEM;
            }

            return STATUS_OK;
        }

        const LSPString *UIOverrides::get(const LSPString *name) const
        {
            // Get attribute list to read
            const attlist_t *list = vStack.last();
            if (list == NULL)
                return NULL;

            // Simple linear search through attributes
            for (size_t i=0, n=list->items.size(); i<n; ++i)
            {
                const attribute_t *att = list->items.uget(i);
                if (att->name.equals(name))
                    return &att->value;
            }

            return NULL;
        }

        const LSPString *UIOverrides::name(size_t index) const
        {
            // Get attribute list to read
            const attlist_t *list = vStack.last();
            if (list == NULL)
                return NULL;

            const attribute_t *att = list->items.get(index);
            return (att != NULL) ? &att->name : NULL;
        }

        const LSPString *UIOverrides::value(size_t index) const
        {
            // Get attribute list to read
            const attlist_t *list = vStack.last();
            if (list == NULL)
                return NULL;

            const attribute_t *att = list->items.get(index);
            return (att != NULL) ? &att->value : NULL;
        }

        size_t UIOverrides::count() const
        {
            // Get attribute list to read
            const attlist_t *list = vStack.last();
            return (list != NULL) ? list->items.size() : 0;
        }

        bool UIOverrides::attribute_present(const LSPString * const *atts, const LSPString *name)
        {
            // Iterate over key=value pairs
            for ( ; *atts != NULL; atts += 2)
            {
                if (name->equals(*atts))
                    return true;
            }
            return false;
        }

        status_t UIOverrides::build(lltl::parray<LSPString> *dst, const LSPString * const *atts)
        {
            lltl::parray<LSPString> tmp;

            // Emit overridden attributes first
            const attlist_t *list = vStack.last();
            if (list != NULL)
            {
                for (size_t i=0, n=list->items.size(); i<n; ++i)
                {
                    attribute_t *att = const_cast<attribute_t *>(list->items.uget(i));
                    if (att == NULL)
                        return STATUS_CORRUPTED;
                    if (attribute_present(atts, &att->name))
                        continue;
                    if (!tmp.add(&att->name))
                        return STATUS_NO_MEM;
                    if (!tmp.add(&att->value))
                        return STATUS_NO_MEM;
                }
            }

            // Now emit other attributes
            for ( ; *atts != NULL; ++atts)
            {
                if (!tmp.add(const_cast<LSPString *>(*atts)))
                    return STATUS_NO_MEM;
            }

            // Append NULL-terminated element
            if (!tmp.add(static_cast<LSPString *>(NULL)))
                return STATUS_NO_MEM;

            // Commit changes
            dst->swap(&tmp);
            return STATUS_OK;
        }

    } // namespace ui
} // namespace lsp


