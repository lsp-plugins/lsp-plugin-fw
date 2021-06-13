/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 июн. 2021 г.
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
        Direction::Direction()
        {
            pWrapper        = NULL;
            pDirection        = NULL;

            for (size_t i=0; i<DIR_COUNT; ++i)
                vExpr[i]    = NULL;
        }

        Direction::~Direction()
        {
            pWrapper    = NULL;
            pDirection    = NULL;

            for (size_t i=0; i<DIR_COUNT; ++i)
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

        status_t Direction::init(ui::IWrapper *wrapper, tk::Vector2D *direction)
        {
            if (pWrapper != NULL)
                return STATUS_BAD_STATE;
            else if (direction == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Save color
            pWrapper    = wrapper;
            pDirection  = direction;

            return STATUS_OK;
        }

        void Direction::apply_change(size_t index, expr::value_t *value)
        {
            // Perform the cast
            if (expr::cast_value(value, expr::VT_FLOAT) != STATUS_OK)
                return;

            // Assign the desired property
            switch (index)
            {
                case DIR_DX:            pDirection->set_dx(value->v_float);         break;
                case DIR_DY:            pDirection->set_dy(value->v_float);         break;
                case DIR_ANGLE:         pDirection->set_angle(value->v_float);      break;
                case DIR_DANGLE:        pDirection->set_dangle(value->v_float);     break;
                case DIR_LENGTH:        pDirection->set_length(value->v_float);     break;
                default: break;
            }
        }

        bool Direction::set(const char *param, const char *name, const char *value)
        {
            if (param == NULL)
                param   = "pad";

            size_t len = strlen(param);
            if (strncmp(param, name, len))
                return false;
            name += len;

            // Decode the attribute
            ssize_t idx     = -1;
            if (!strcmp(name, ".dx"))               idx = DIR_DX;
            else if (!strcmp(name, ".hor"))         idx = DIR_DX;
            else if (!strcmp(name, ".horizontal"))  idx = DIR_DX;
            else if (!strcmp(name, ".dy"))          idx = DIR_DY;
            else if (!strcmp(name, ".vert"))        idx = DIR_DY;
            else if (!strcmp(name, ".vertical"))    idx = DIR_DY;
            else if (!strcmp(name, ".rho"))         idx = DIR_LENGTH;
            else if (!strcmp(name, ".r"))           idx = DIR_LENGTH;
            else if (!strcmp(name, ".len"))         idx = DIR_LENGTH;
            else if (!strcmp(name, ".length"))      idx = DIR_LENGTH;
            else if (!strcmp(name, ".phi"))         idx = DIR_ANGLE;
            else if (!strcmp(name, ".rphi"))        idx = DIR_ANGLE;
            else if (!strcmp(name, ".rad"))         idx = DIR_ANGLE;
            else if (!strcmp(name, ".radians"))     idx = DIR_ANGLE;
            else if (!strcmp(name, ".dphi"))        idx = DIR_DANGLE;
            else if (!strcmp(name, ".deg"))         idx = DIR_DANGLE;
            else if (!strcmp(name, ".degrees"))     idx = DIR_DANGLE;
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

        void Direction::notify(ui::IPort *port)
        {
            if (pDirection == NULL)
                return;

            expr::value_t value;
            expr::init_value(&value);

            for (size_t i=0; i<DIR_COUNT; ++i)
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





