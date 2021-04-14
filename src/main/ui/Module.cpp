/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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
        Module::Module(const meta::plugin_t *meta)
        {
            pMetadata       = meta;
            pWrapper        = NULL;
            pDisplay        = NULL;
            wRoot           = NULL;
        }

        Module::~Module()
        {
            do_destroy();
        }

        void Module::destroy()
        {
            do_destroy();
        }

        void Module::do_destroy()
        {
            sMapping.flush();

            // Destroy all widgets in reverse order
            for (size_t i=vWidgets.size(); (i--) > 0;)
            {
                tk::Widget *w = vWidgets.uget(i);
                if (w != NULL)
                {
                    w->destroy();
                    delete w;
                }
            }
            vWidgets.flush();

            // Forget the root widget
            wRoot       = NULL;
        }

        status_t Module::init(IWrapper *wrapper, tk::Display *dpy)
        {
            pWrapper        = wrapper;
            pDisplay        = dpy;

            return STATUS_OK;
        }

        void Module::position_updated(const plug::position_t *pos)
        {
        }

        void Module::sync_meta_ports()
        {
        }

        void Module::kvt_write(core::KVTStorage *storage, const char *id, const core::kvt_param_t *value)
        {
        }

        status_t Module::add_widget(tk::Widget *w)
        {
            if (w == NULL)
                return STATUS_BAD_ARGUMENTS;
            return (vWidgets.add(w) != NULL) ? STATUS_OK : STATUS_NO_MEM;
        }

        status_t Module::map_widget(const char *uid, tk::Widget *w)
        {
            if (sMapping.create(uid, w))
                return STATUS_OK;
            return (sMapping.contains(uid)) ? STATUS_ALREADY_EXISTS : STATUS_NO_MEM;
        }

        status_t Module::map_widget(const LSPString *uid, tk::Widget *w)
        {
            return (uid != NULL) ? map_widget(uid->get_utf8(), w) : STATUS_BAD_ARGUMENTS;
        }

        tk::Widget *Module::find_widget(const char *uid)
        {
            return sMapping.get(uid);
        }

        tk::Widget *Module::find_widget(const LSPString *uid)
        {
            return (uid != NULL) ? find_widget(uid->get_utf8()) : NULL;
        }

        status_t Module::unmap_widget(const char *uid)
        {
            return (sMapping.remove(uid, NULL)) ? STATUS_OK : STATUS_NOT_FOUND;
        }

        status_t Module::unmap_widget(const LSPString *uid)
        {
            return unmap_widget(uid->get_utf8());
        }

        status_t Module::unmap_widget(const tk::Widget *w)
        {
            // TODO: add two-way mapping
            lltl::parray<char> keys;
            lltl::parray<tk::Widget> values;

            if (!sMapping.items(&keys, &values))
                return STATUS_NO_MEM;

            for (size_t i=0, n=values.size(); i<n; ++i)
            {
                tk::Widget *xw = values.uget(i);
                if (w == xw)
                {
                    sMapping.remove(keys.uget(i), NULL);
                    return STATUS_OK;
                }
            }

            return STATUS_NOT_FOUND;
        }

        ssize_t Module::unmap_widgets(const tk::Widget * const *w, size_t n)
        {
            lltl::parray<char> keys;
            lltl::parray<tk::Widget> values;
            lltl::parray<tk::Widget> list;

            if (!sMapping.items(&keys, &values))
                return -STATUS_NO_MEM;

            // Prepare sorted list of widgets
            if (!list.add_n(n, const_cast<tk::Widget **>(w)))
                return -STATUS_NO_MEM;
            list.qsort(lltl::ptr_cmp_func);

            // Do main loop
            size_t unmapped = 0;
            for (size_t i=0, n=values.size(); i<n; ++i)
            {
                tk::Widget *xw = values.uget(i);
                if (remove_item(&list, xw))
                {
                    sMapping.remove(keys.uget(i), NULL);
                    ++unmapped;
                }
            }

            return unmapped;
        }

        bool Module::remove_item(lltl::parray<tk::Widget> *slist, tk::Widget *w)
        {
            // Use binary search
            ssize_t first = 0, last = slist->size() - 1;
            while (first <= last)
            {
                ssize_t mid = (first + last) >> 1;
                tk::Widget *c = slist->uget(mid);

                if (w < c)
                    last        = mid - 1;
                else if (w > c)
                    first       = mid + 1;
                else
                {
                    slist->remove(mid);
                    return true;
                }
            }

            return false;
        }
    }
}


