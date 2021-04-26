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

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace ctl
    {
        Embedding::Embedding()
        {
            pEmbedding      = NULL;
            pWrapper    = NULL;
            for (size_t i=0; i<E_TOTAL; ++i)
                vExpr[i]    = NULL;
        }

        Embedding::~Embedding()
        {
            for (size_t i=0; i<E_TOTAL; ++i)
            {
                Expression *e = vExpr[i];
                if (e != NULL)
                {
                    e->destroy();
                    delete e;
                    vExpr[i]    = NULL;
                }
            }

            pEmbedding      = NULL;
            pWrapper        = NULL;
        }

        status_t Embedding::init(ui::IWrapper *wrapper, tk::Embedding *color)
        {
            if (pEmbedding != NULL)
                return STATUS_BAD_STATE;
            else if (color == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Save color
            pEmbedding      = color;
            pWrapper        = wrapper;

            return STATUS_OK;
        }

        bool Embedding::set(const char *prop, const char *name, const char *value)
        {
            if (prop == NULL)
                return false;

            ssize_t idx = -1;
            size_t len = strlen(prop);

            if (strncmp(name, prop, len))
                return false;
            name += len;

            if (name[0] == '\0')
                idx         = E_ALL;
            else if (name[0] == '.')
            {
                ++name;

                if      (!strcmp(name, "h"))        idx = E_H;
                else if (!strcmp(name, "hor"))      idx = E_H;
                else if (!strcmp(name, "v"))        idx = E_V;
                else if (!strcmp(name, "vert"))     idx = E_V;
                else if (!strcmp(name, "l"))        idx = E_L;
                else if (!strcmp(name, "left"))     idx = E_L;
                else if (!strcmp(name, "r"))        idx = E_R;
                else if (!strcmp(name, "right"))    idx = E_R;
                else if (!strcmp(name, "t"))        idx = E_T;
                else if (!strcmp(name, "top"))      idx = E_T;
                else if (!strcmp(name, "b"))        idx = E_B;
                else if (!strcmp(name, "bottom"))   idx = E_B;
            }
            else
                return false;

            if (idx < 0)
                return false;

            // Create the corresponding expression
            ctl::Expression *e = vExpr[idx];
            if (e == NULL)
            {
                e       = new ctl::Expression();
                if (e == NULL)
                    return false;
                e->init(pWrapper, this);
                vExpr[idx]      = e;
            }

            // Finally, parse the expression
            return e->parse(value) == STATUS_OK;
        }

        void Embedding::notify(ui::IPort *port)
        {
            if (pEmbedding == NULL)
                return;

            expr::value_t value;
            expr::init_value(&value);

            for (size_t i=0; i<E_TOTAL; ++i)
            {
                // Evaluate the expression
                ctl::Expression *e = vExpr[i];
                if ((e == NULL) || (!e->depends(port)))
                    continue;
                if (e->evaluate(&value) != STATUS_OK)
                    continue;

                // Perform the cast
                if (expr::cast_value(&value, expr::VT_BOOL) != STATUS_OK)
                    continue;

                // Assign the desired property
                switch (i)
                {
                    case E_ALL:     pEmbedding->set(value.v_bool);              break;
                    case E_H:       pEmbedding->set_horizontal(value.v_bool);   break;
                    case E_V:       pEmbedding->set_vertical(value.v_bool);     break;
                    case E_L:       pEmbedding->set_left(value.v_bool);         break;
                    case E_R:       pEmbedding->set_right(value.v_bool);        break;
                    case E_T:       pEmbedding->set_top(value.v_bool);          break;
                    case E_B:       pEmbedding->set_bottom(value.v_bool);       break;
                    default: break;
                }
            }

            expr::destroy_value(&value);
        }
    }
}



