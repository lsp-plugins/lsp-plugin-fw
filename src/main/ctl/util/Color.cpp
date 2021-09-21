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
            if (pWrapper != NULL)
                pWrapper->remove_schema_listener(this);

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

            // Save color and wrapper
            pColor      = color;
            pWrapper    = wrapper;

            return pWrapper->add_schema_listener(this);
        }

        Color::control_t Color::get_control(const char *property, control_t dfl) const
        {
            if (pWrapper == NULL)
                return dfl;
            tk::Display *dpy = pWrapper->display();
            if (dpy == NULL)
                return dfl;
            tk::Schema *schema = dpy->schema();
            if (schema == NULL)
                return dfl;
            tk::Style *style = schema->root();
            if (style == NULL)
                return dfl;

            LSPString value;
            if (style->get_string(property, &value) != STATUS_OK)
                return dfl;

            if (value.equals_ascii_nocase("hsl"))
                return CTL_HSL;
            if (value.equals_ascii_nocase("hcl"))
                return CTL_LCH;
            if (value.equals_ascii_nocase("lch"))
                return CTL_LCH;

            return dfl;
        }

        void Color::apply_change(size_t index, expr::value_t *value)
        {
            // Perform the cast
            expr::value_type_t vt  = (index == C_VALUE) ? expr::VT_STRING : expr::VT_FLOAT;
            if (expr::cast_value(value, vt) != STATUS_OK)
                return;

            // Pre-process index
            switch (index)
            {
                case C_CTL_HUE:
                    index = (get_control("color.hue.control", CTL_LCH) == CTL_LCH) ? C_LCH_H: C_HSL_H;
                    break;
                case C_CTL_LIGHT:
                    index = (get_control("color.lightness.control", CTL_LCH) == CTL_LCH) ? C_LCH_L: C_HSL_L;
                    break;
                case C_CTL_SAT:
                    index = (get_control("color.saturation.control", CTL_LCH) == CTL_LCH) ? C_LCH_C: C_HSL_S;
                    break;
            }

            // Assign the desired property
            switch (index)
            {
                case C_VALUE:       pColor->set(value->v_str);              break;
                case C_RGB_R:       pColor->red(value->v_float);            break;
                case C_RGB_G:       pColor->green(value->v_float);          break;
                case C_RGB_B:       pColor->blue(value->v_float);           break;
                case C_HSL_H:       pColor->hsl_hue(value->v_float);        break;
                case C_HSL_S:       pColor->hsl_saturation(value->v_float); break;
                case C_HSL_L:       pColor->hsl_lightness(value->v_float);  break;
                case C_XYZ_X:       pColor->xyz_x(value->v_float);          break;
                case C_XYZ_Y:       pColor->xyz_y(value->v_float);          break;
                case C_XYZ_Z:       pColor->xyz_z(value->v_float);          break;
                case C_LAB_L:       pColor->lab_l(value->v_float);          break;
                case C_LAB_A:       pColor->lab_a(value->v_float);          break;
                case C_LAB_B:       pColor->lab_b(value->v_float);          break;
                case C_LCH_L:       pColor->lch_l(value->v_float);          break;
                case C_LCH_C:       pColor->lch_c(value->v_float);          break;
                case C_LCH_H:       pColor->lch_h(value->v_float * 360.0f); break;
                case C_ALPHA:       pColor->alpha(value->v_float);          break;
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

                if (!strncmp(name, ".rgb", 4))
                {
                    // '.rgb' sub-prefix
                    name       += 4;
                    if      (!strcmp(name, ".red"))         idx = C_RGB_R;
                    else if (!strcmp(name, ".r"))           idx = C_RGB_R;
                    else if (!strcmp(name, ".green"))       idx = C_RGB_G;
                    else if (!strcmp(name, ".g"))           idx = C_RGB_G;
                    else if (!strcmp(name, ".blue"))        idx = C_RGB_B;
                    else if (!strcmp(name, ".b"))           idx = C_RGB_B;
                }
                else if (!strncmp(name, ".hsl", 4))
                {
                    // '.hsl' sub-prefix
                    name       += 4;
                    if      (!strcmp(name, ".hue"))         idx = C_HSL_H;
                    else if (!strcmp(name, ".h"))           idx = C_HSL_H;
                    else if (!strcmp(name, ".saturation"))  idx = C_HSL_S;
                    else if (!strcmp(name, ".sat"))         idx = C_HSL_S;
                    else if (!strcmp(name, ".s"))           idx = C_HSL_S;
                    else if (!strcmp(name, ".lightness"))   idx = C_HSL_L;
                    else if (!strcmp(name, ".light"))       idx = C_HSL_L;
                    else if (!strcmp(name, ".l"))           idx = C_HSL_L;
                }
                else if (!strncmp(name, ".xyz", 4))
                {
                    // '.xyz' sub-prefix
                    name       += 4;
                    if      (!strcmp(name, ".x"))           idx = C_XYZ_X;
                    else if (!strcmp(name, ".luminance"))   idx = C_XYZ_Y;
                    else if (!strcmp(name, ".lum"))         idx = C_XYZ_Y;
                    else if (!strcmp(name, ".y"))           idx = C_XYZ_Y;
                    else if (!strcmp(name, ".z"))           idx = C_XYZ_Z;
                }
                else if (!strncmp(name, ".lab", 4))
                {
                    // '.lab' sub-prefix
                    name       += 4;
                    if      (!strcmp(name, ".luminance"))   idx = C_LAB_L;
                    else if (!strcmp(name, ".lum"))         idx = C_LAB_L;
                    else if (!strcmp(name, ".l"))           idx = C_LAB_L;
                    else if (!strcmp(name, ".a"))           idx = C_LAB_A;
                    else if (!strcmp(name, ".b"))           idx = C_LAB_B;
                }
                else if ((!strncmp(name, ".lch", 4)) || (!strncmp(name, ".hcl", 4)))
                {
                    // '.lch' sub-prefix
                    name       += 4;
                    if      (!strcmp(name, ".luminance"))   idx = C_LCH_L;
                    else if (!strcmp(name, ".lum"))         idx = C_LCH_L;
                    else if (!strcmp(name, ".lightness"))   idx = C_LCH_L;
                    else if (!strcmp(name, ".light"))       idx = C_LCH_L;
                    else if (!strcmp(name, ".l"))           idx = C_LCH_L;
                    else if (!strcmp(name, ".chroma"))      idx = C_LCH_C;
                    else if (!strcmp(name, ".c"))           idx = C_LCH_C;
                    else if (!strcmp(name, ".hue"))         idx = C_LCH_H;
                    else if (!strcmp(name, ".h"))           idx = C_LCH_H;
                }
                else
                {
                    // No sub-prefix
                    if      (!strcmp(name, ".red"))         idx = C_RGB_R;
                    else if (!strcmp(name, ".r"))           idx = C_RGB_R;
                    else if (!strcmp(name, ".green"))       idx = C_RGB_G;
                    else if (!strcmp(name, ".g"))           idx = C_RGB_G;
                    else if (!strcmp(name, ".blue"))        idx = C_RGB_B;
                    else if (!strcmp(name, ".b"))           idx = C_RGB_B;

                    else if (!strcmp(name, ".hue"))         idx = C_CTL_HUE;
                    else if (!strcmp(name, ".h"))           idx = C_CTL_HUE;

                    else if (!strcmp(name, ".sat"))         idx = C_CTL_SAT;
                    else if (!strcmp(name, ".saturation"))  idx = C_CTL_SAT;
                    else if (!strcmp(name, ".s"))           idx = C_CTL_SAT;
                    else if (!strcmp(name, ".lightness"))   idx = C_CTL_LIGHT;
                    else if (!strcmp(name, ".light"))       idx = C_CTL_LIGHT;
                    else if (!strcmp(name, ".l"))           idx = C_CTL_LIGHT;

                    else if (!strcmp(name, ".luminance"))   idx = C_CTL_LIGHT;
                    else if (!strcmp(name, ".lum"))         idx = C_CTL_LIGHT;
                    else if (!strcmp(name, ".chroma"))      idx = C_CTL_SAT;
                    else if (!strcmp(name, ".c"))           idx = C_CTL_SAT;

                    else if (!strcmp(name, ".alpha"))       idx = C_ALPHA;
                    else if (!strcmp(name, ".a"))           idx = C_ALPHA;
                }
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
            if (!e->parse(value, EXPR_FLAGS_NONE))
            {
                if (idx != C_VALUE)
                    return false;
                if (!e->parse(value, EXPR_FLAGS_STRING))
                    return false;
            }

            // And apply the computed value
            expr::value_t cv;
            expr::init_value(&cv);

            if (e->evaluate(&cv) == STATUS_OK)
            {
                apply_change(idx, &cv);

                // Evaluate other particular expressions
                if (idx == C_VALUE)
                {
                    while (++idx < C_TOTAL)
                    {
                        e   = vExpr[idx];
                        if (e == NULL)
                            continue;
                        if (e->evaluate(&cv) == STATUS_OK)
                            apply_change(idx, &cv);
                    }
                }
            }

            expr::destroy_value(&cv);

            return true;
        }

        void Color::notify(ui::IPort *port)
        {
            if (pColor == NULL)
                return;

            expr::value_t value;
            expr::init_value(&value);

            bool all = (vExpr[C_VALUE] != NULL) && (vExpr[C_VALUE]->depends(port));

            if (all)
            {
                // The main value has changed, need to re-evaluate all components
                for (size_t i=0; i<C_TOTAL; ++i)
                {
                    // Evaluate the expression
                    Expression *e = vExpr[i];
                    if ((e == NULL) || (!e->valid()))
                        continue;
                    if (e->evaluate(&value) == STATUS_OK)
                        apply_change(i, &value);
                }
            }
            else
            {
                // Re-evaluate the dependent components
                for (size_t i=0; i<C_TOTAL; ++i)
                {
                    // Evaluate the expression
                    Expression *e = vExpr[i];
                    if ((e == NULL) || (!e->depends(port)))
                        continue;
                    if (e->evaluate(&value) == STATUS_OK)
                        apply_change(i, &value);
                }
            }

            expr::destroy_value(&value);
        }

        void Color::reload()
        {
            if (pColor == NULL)
                return;

            pColor->set_default();

            expr::value_t value;
            expr::init_value(&value);

            for (size_t i=0; i<C_TOTAL; ++i)
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

        void Color::reloaded(const tk::StyleSheet *sheet)
        {
            reload();
        }
    }
}


