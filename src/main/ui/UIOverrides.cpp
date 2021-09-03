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

        void UIOverrides::drop_attlist(attlist_t *list)
        {
            for (size_t i=0, n=list->size(); i<n; ++i)
            {
                attribute_t *att = list->uget(i);
                if ((-- att->refs) <= 0)
                    delete att;
            }
            list->flush();
        }

        status_t UIOverrides::push()
        {
            attlist_t *list = new attlist_t;
            if (list == NULL)
                return STATUS_NO_MEM;

            // Check that we need to copy the attribute list
            attlist_t *src = vStack.last();
            if (src == NULL)
                return STATUS_OK;

            // We need to make a copy of current list
            list->reserve(src->size());
            for (size_t i=0, n=src->size(); i<n; ++i)
            {
                attribute_t *att = list->uget(i);

                // Skip attributes with limited visibility depth
                if ((att->depth >= 0) && (att->refs >= att->depth))
                    continue;

                // Try to add item to the list
                if (!list->add(att))
                {
                    drop_attlist(list);
                    return STATUS_NO_MEM;
                }

                // Increate the reference counter
                ++att->refs;
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
            att->depth  = depth;

            // Replace existing attribute if it is present
            for (size_t i=0, n=list->size(); i<n; ++i)
            {
                attribute_t *curr = list->uget(i);
                if (curr == NULL)
                {
                    delete att;
                    return STATUS_CORRUPTED;
                }

                if (!curr->name.equals(&att->name))
                    continue;

                // Try to replace the attribute
                if (!list->set(i, att))
                {
                    delete att;
                    return STATUS_NO_MEM;
                }

                // Dereference replaced attribute
                if ((--curr->refs) <= 0)
                    delete curr;

                // Attribute has been added
                return STATUS_OK;
            }

            // There is no attribute with such name present, add new
            if (!list->add(att))
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
            for (size_t i=0, n=list->size(); i<n; ++i)
            {
                const attribute_t *att = list->uget(i);
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

            const attribute_t *att = list->get(index);
            return (att != NULL) ? &att->name : NULL;
        }

        const LSPString *UIOverrides::value(size_t index) const
        {
            // Get attribute list to read
            const attlist_t *list = vStack.last();
            if (list == NULL)
                return NULL;

            const attribute_t *att = list->get(index);
            return (att != NULL) ? &att->value : NULL;
        }

        size_t UIOverrides::count() const
        {
            // Get attribute list to read
            const attlist_t *list = vStack.last();
            return (list != NULL) ? list->size() : 0;
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
                for (size_t i=0, n=list->size(); i<n; ++i)
                {
                    attribute_t *att = const_cast<attribute_t *>(list->uget(i));
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
            for ( ; *atts != NULL; atts += 2)
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


