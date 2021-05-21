/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 апр. 2021 г.
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
        Color::Color()
        {
            pColor      = NULL;
            pWrapper    = NULL;
            for (size_t i=0; i<C_TOTAL; ++i)
                vExpr[i]    = NULL;
        }

        Color::~Color()
        {
            for (size_t i=0; i<C_TOTAL; ++i)
            {
                Expression *e = vExpr[i];
                if (e != NULL)
                {
                    e->destroy();
                    delete e;
                    vExpr[i]    = NULL;
                }
            }

            pColor      = NULL;
            pWrapper    = NULL;
        }

        status_t Color::init(ui::IWrapper *wrapper, tk::Color *color)
        {
            if (pColor != NULL)
                return STATUS_BAD_STATE;
            else if (color == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Save color
            pColor      = color;
            pWrapper    = wrapper;

            return STATUS_OK;
        }

        void Color::apply_change(size_t index, expr::value_t *value)
        {
            // Perform the cast
            expr::value_type_t vt  = (index == C_VALUE) ? expr::VT_STRING : expr::VT_FLOAT;
            if (expr::cast_value(value, vt) != STATUS_OK)
                return;

            // Assign the desired property
            switch (index)
            {
                case C_VALUE:   pColor->set(value->v_str);          break;
                case C_R:       pColor->red(value->v_float);        break;
                case C_G:       pColor->green(value->v_float);      break;
                case C_B:       pColor->blue(value->v_float);       break;
                case C_H:       pColor->hue(value->v_float);        break;
                case C_S:       pColor->saturation(value->v_float); break;
                case C_L:       pColor->lightness(value->v_float);  break;
                case C_A:       pColor->alpha(value->v_float);      break;
                default: break;
            }
        }

        bool Color::set(const char *prefix, const char *name, const char *value)
        {
            ssize_t idx     = -1;
            size_t len      = strlen(prefix);

            if (!strcmp(name, prefix))
                idx         = C_VALUE;
            else if (!strncmp(name, prefix, len))
            {
                name       += strlen(prefix);

                if      (!strcmp(name, ".red"))     idx = C_R;
                else if (!strcmp(name, ".r"))       idx = C_R;
                else if (!strcmp(name, ".green"))   idx = C_G;
                else if (!strcmp(name, ".g"))       idx = C_G;
                else if (!strcmp(name, ".blue"))    idx = C_B;
                else if (!strcmp(name, ".b"))       idx = C_B;
                else if (!strcmp(name, ".hue"))     idx = C_H;
                else if (!strcmp(name, ".h"))       idx = C_H;
                else if (!strcmp(name, ".sat"))     idx = C_S;
                else if (!strcmp(name, ".s"))       idx = C_S;
                else if (!strcmp(name, ".light"))   idx = C_L;
                else if (!strcmp(name, ".l"))       idx = C_L;
                else if (!strcmp(name, ".alpha"))   idx = C_A;
                else if (!strcmp(name, ".a"))       idx = C_A;
            }

            if (idx < 0)
                return false;

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
            size_t flags = (idx == C_VALUE) ? EXPR_FLAGS_STRING : 0;
            if (!e->parse(value, flags))
                return false;

            // And apply the computed value
            expr::value_t cv;
            expr::init_value(&cv);

            if (e->evaluate(&cv) == STATUS_OK)
                apply_change(idx, &cv);

            expr::destroy_value(&cv);

            return true;
        }

        void Color::notify(ui::IPort *port)
        {
            if (pColor == NULL)
                return;

            expr::value_t value;
            expr::init_value(&value);

            for (size_t i=0; i<C_TOTAL; ++i)
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
    }
}


