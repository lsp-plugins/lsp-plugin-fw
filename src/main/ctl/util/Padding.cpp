/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 13 мая 2021 г.
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
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace ctl
    {
        Padding::Padding()
        {
            pWrapper        = NULL;
            pPadding        = NULL;

            for (size_t i=0; i<PAD_COUNT; ++i)
                vExpr[i]    = NULL;
        }

        Padding::~Padding()
        {
            if (pWrapper != NULL)
                pWrapper->remove_schema_listener(this);

            pWrapper    = NULL;
            pPadding    = NULL;

            for (size_t i=0; i<PAD_COUNT; ++i)
            {
                Expression *e = vExpr[i];
                if (e != NULL)
                {
                    e->destroy();
                    delete e;
                    vExpr[i]    = NULL;
                }
            }
        }

        status_t Padding::init(ui::IWrapper *wrapper, tk::Padding *padding)
        {
            if (pWrapper != NULL)
                return STATUS_BAD_STATE;
            else if (padding == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Save color
            pWrapper    = wrapper;
            pPadding    = padding;

            return pWrapper->add_schema_listener(this);
        }

        void Padding::apply_change(size_t index, expr::value_t *value)
        {
            // Perform the cast
            if (expr::cast_value(value, expr::VT_INT) != STATUS_OK)
                return;

            // Assign the desired property
            switch (index)
            {
                case PAD_ALL:           pPadding->set_all(value->v_int);        break;
                case PAD_LEFT:          pPadding->set_left(value->v_int);       break;
                case PAD_RIGHT:         pPadding->set_right(value->v_int);      break;
                case PAD_TOP:           pPadding->set_top(value->v_int);        break;
                case PAD_BOTTOM:        pPadding->set_bottom(value->v_int);     break;
                case PAD_HORIZONTAL:    pPadding->set_horizontal(value->v_int); break;
                case PAD_VERTICAL:      pPadding->set_vertical(value->v_int);   break;
                default: break;
            }
        }

        bool Padding::set(const char *param, const char *name, const char *value)
        {
            if (param == NULL)
                param   = "pad";

            size_t len = strlen(param);
            if (strncmp(param, name, len))
                return false;
            name += len;

            // Decode the attribute
            ssize_t idx     = -1;
            if (name[0] == '\0')                    idx = PAD_ALL;
            else if (!strcmp(name, ".l"))           idx = PAD_LEFT;
            else if (!strcmp(name, ".left"))        idx = PAD_LEFT;
            else if (!strcmp(name, ".r"))           idx = PAD_RIGHT;
            else if (!strcmp(name, ".right"))       idx = PAD_RIGHT;
            else if (!strcmp(name, ".t"))           idx = PAD_TOP;
            else if (!strcmp(name, ".top"))         idx = PAD_TOP;
            else if (!strcmp(name, ".b"))           idx = PAD_BOTTOM;
            else if (!strcmp(name, ".bottom"))      idx = PAD_BOTTOM;
            else if (!strcmp(name, ".h"))           idx = PAD_HORIZONTAL;
            else if (!strcmp(name, ".hor"))         idx = PAD_HORIZONTAL;
            else if (!strcmp(name, ".horizontal"))  idx = PAD_HORIZONTAL;
            else if (!strcmp(name, ".v"))           idx = PAD_VERTICAL;
            else if (!strcmp(name, ".vert"))        idx = PAD_VERTICAL;
            else if (!strcmp(name, ".vertical"))    idx = PAD_VERTICAL;
            else return false;

            // Create the corresponding expression
            Expression *e = vExpr[idx];
            if (e == NULL)
            {
                e       = new Expression();
                if (e == NULL)
                    return false;
                e->init(pWrapper, this);
                vExpr[idx]      = e;
            }

            // Finally, parse the expression
            if (!e->parse(value, 0))
                return false;

            // And apply the computed value
            expr::value_t cv;
            expr::init_value(&cv);

            if (e->evaluate(&cv) == STATUS_OK)
                apply_change(idx, &cv);

            expr::destroy_value(&cv);

            return true;
        }

        void Padding::notify(ui::IPort *port)
        {
            if (pPadding == NULL)
                return;

            expr::value_t value;
            expr::init_value(&value);

            for (size_t i=0; i<PAD_COUNT; ++i)
            {
                // Evaluate the expression
                Expression *e = vExpr[i];
                if ((e == NULL) || (!e->depends(port)))
                    continue;
                if (e->evaluate(&value) == STATUS_OK)
                    apply_change(i, &value);
            }

            expr::destroy_value(&value);
        }

        void Padding::reloaded(const tk::StyleSheet *sheet)
        {
            ISchemaListener::reloaded(sheet);

            if (pPadding == NULL)
                return;

            expr::value_t value;
            expr::init_value(&value);

            for (size_t i=0; i<PAD_COUNT; ++i)
            {
                // Evaluate the expression
                Expression *e = vExpr[i];
                if ((e == NULL) || (!e->valid()))
                    continue;
                if (e->evaluate(&value) == STATUS_OK)
                    apply_change(i, &value);
            }

            expr::destroy_value(&value);
        }
    }
}


