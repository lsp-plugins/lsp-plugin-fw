/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 апр. 2021 г.
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
        Float::Float()
        {
            pProp       = NULL;
        }

        Float::~Float()
        {
            if (pWrapper != NULL)
                pWrapper->remove_schema_listener(this);
        }

        void Float::on_updated(ui::IPort *port)
        {
            apply_changes();
        }

        void Float::reloaded(const tk::StyleSheet *sheet)
        {
            apply_changes();
        }

        void Float::apply_changes()
        {
            if (pProp == NULL)
                return;

            expr::value_t value;
            expr::init_value(&value);

            if (evaluate(&value) == STATUS_OK)
            {
                if (expr::cast_float(&value) == STATUS_OK)
                    pProp->set(value.v_float);
            }

            expr::destroy_value(&value);
        }

        void Float::init(ui::IWrapper *wrapper, tk::Float *prop)
        {
            Property::init(wrapper);
            pProp       = prop;

            if (pWrapper != NULL)
                pWrapper->add_schema_listener(this);
        }

        bool Float::set(const char *prop, const char *name, const char *value)
        {
            if (strcmp(prop, name))
                return false;

            if (!Property::parse(value))
                return false;

            apply_changes();
            return true;
        }

    }
}

