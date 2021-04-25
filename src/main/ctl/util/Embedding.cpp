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
            pWrapper    = NULL;
        }

        status_t Embedding::init(ui::IWrapper *wrapper, tk::Embedding *color, const char *prefix)
        {
            if (pEmbedding != NULL)
                return STATUS_BAD_STATE;
            else if (color == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Save prefix
            if (prefix != NULL)
            {
                if (!sPrefix.set_utf8(prefix))
                    return STATUS_NO_MEM;
            }

            // Save color
            pEmbedding      = color;
            pWrapper    = wrapper;

            return STATUS_OK;
        }

        status_t Embedding::init(ui::IWrapper *wrapper, tk::Embedding *color, const LSPString *prefix)
        {
            return init(wrapper, color, (prefix != NULL) ? prefix->get_utf8() : NULL);
        }

        void Embedding::set(const char *name, const char *value)
        {
            const char *prefix = sPrefix.get_utf8();
            ssize_t idx = -1;

            if (!strcmp(name, prefix))
                idx         = E_ALL;
            else if (strstr(name, prefix) == name)
            {
                name       += strlen(prefix);

                if      (!strcmp(name, ".h"))       idx = E_H;
                else if (!strcmp(name, ".hor"))     idx = E_H;
                else if (!strcmp(name, ".v"))       idx = E_V;
                else if (!strcmp(name, ".vert"))    idx = E_V;
                else if (!strcmp(name, ".l"))       idx = E_L;
                else if (!strcmp(name, ".left"))    idx = E_L;
                else if (!strcmp(name, ".r"))       idx = E_R;
                else if (!strcmp(name, ".right"))   idx = E_R;
                else if (!strcmp(name, ".t"))       idx = E_T;
                else if (!strcmp(name, ".top"))     idx = E_T;
                else if (!strcmp(name, ".b"))       idx = E_B;
                else if (!strcmp(name, ".bottom"))  idx = E_B;
            }

            if (idx < 0)
                return;

            // Create the corresponding expression
            Expression *e = vExpr[idx];
            if (e == NULL)
            {
                e       = new Expression();
                if (e == NULL)
                    return;
                e->init(pWrapper, this);
                vExpr[idx]      = e;
            }

            // Finally, parse the expression
            e->parse(value);
        }

        void Embedding::notify(ui::IPort *port)
        {
            if (pEmbedding == NULL)
                return;

            expr::value_t value;
            expr::value_type_t vt;
            expr::init_value(&value);

            for (size_t i=0; i<E_TOTAL; ++i)
            {
                // Evaluate the expression
                Expression *e = vExpr[i];
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



