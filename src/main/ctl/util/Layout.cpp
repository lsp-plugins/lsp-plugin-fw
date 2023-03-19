/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-para-equalizer
 * Created on: 13 февр. 2023 г.
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

#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        Layout::Layout()
        {
            pLayout     = NULL;
            pWrapper    = NULL;
        }

        Layout::~Layout()
        {
            if (pWrapper != NULL)
                pWrapper->remove_schema_listener(this);
        }

        void Layout::apply_changes()
        {
            if (pLayout == NULL)
                return;

            if (sHAlign.valid())
                pLayout->set_halign(sHAlign.evaluate_float(0.0f));
            if (sVAlign.valid())
                pLayout->set_valign(sVAlign.evaluate_float(0.0f));
            if (sHScale.valid())
                pLayout->set_hscale(sHScale.evaluate_float(0.0f));
            if (sVScale.valid())
                pLayout->set_vscale(sVScale.evaluate_float(0.0f));
        }

        void Layout::notify(ui::IPort *port)
        {
            if ((sHAlign.depends(port)) ||
                (sVAlign.depends(port)) ||
                (sHScale.depends(port)) ||
                (sVScale.depends(port)))
                apply_changes();
        }

        void Layout::init(ui::IWrapper *wrapper, tk::Layout *layout)
        {
            pLayout     = layout;
            pWrapper    = wrapper;

            sHAlign.init(pWrapper, this);
            sVAlign.init(pWrapper, this);
            sHScale.init(pWrapper, this);
            sVScale.init(pWrapper, this);

            pWrapper->add_schema_listener(this);
        }

        bool Layout::set(const char *name, const char *value)
        {
            if (!strcmp(name, "align"))
            {
                bool ok = true;
                if (!sHAlign.parse(value))
                    ok = false;
                if (!sVAlign.parse(value))
                    ok = false;
                return ok;
            }
            if (!strcmp(name, "scale"))
            {
                bool ok = true;
                if (!sHScale.parse(value))
                    ok = false;
                if (!sVScale.parse(value))
                    ok = false;
                return ok;
            }

            if (!strcmp(name, "halign"))
                return sHAlign.parse(value);
            if (!strcmp(name, "valign"))
                return sVAlign.parse(value);
            if (!strcmp(name, "hscale"))
                return sHScale.parse(value);
            if (!strcmp(name, "vscale"))
                return sVScale.parse(value);

            return false;
        }

        void Layout::reloaded(const tk::StyleSheet *sheet)
        {
            apply_changes();
        }

    } /* namespace ctl */
} /* namespace lsp */

