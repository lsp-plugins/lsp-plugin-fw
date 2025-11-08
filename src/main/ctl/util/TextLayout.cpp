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
        TextLayout::TextLayout()
        {
            pLayout     = NULL;
            pWrapper    = NULL;
        }

        TextLayout::~TextLayout()
        {
            if (pWrapper != NULL)
                pWrapper->remove_schema_listener(this);
        }

        void TextLayout::apply_changes()
        {
            if (pLayout == NULL)
                return;

            if (sHAlign.valid())
                pLayout->set_halign(sHAlign.evaluate_float(0.0f));
            if (sVAlign.valid())
                pLayout->set_valign(sVAlign.evaluate_float(0.0f));
        }

        void TextLayout::notify(ui::IPort *port, size_t flags)
        {
            if ((sHAlign.depends(port)) ||
                (sVAlign.depends(port)))
                apply_changes();
        }

        void TextLayout::init(ui::IWrapper *wrapper, tk::TextLayout *layout)
        {
            pLayout     = layout;
            pWrapper    = wrapper;

            sHAlign.init(pWrapper, this);
            sVAlign.init(pWrapper, this);

            pWrapper->add_schema_listener(this);
        }

        bool TextLayout::parse_and_apply(ctl::Expression *expr, const char *value)
        {
            if (!expr->parse(value))
                return false;
            apply_changes();
            return true;
        }

        bool TextLayout::set(const char *name, const char *value)
        {
            switch (name[0])
            {
                case 'h':
                    if (!strncmp(&name[1], "text", 4))
                        return parse_and_apply(&sHAlign, value);
                    break;

                case 'v':
                    if (!strncmp(&name[1], "text", 4))
                        return parse_and_apply(&sVAlign, value);
                    break;

                case 't':
                    if (strncmp(&name[1], "ext.", 4) != 0)
                        return false;
                    name += 5;
                    break;
                default:
                    return false;
            }

            switch (name[0])
            {
                case 'h':
                    if ((name[1] == '\0') || (!strncmp(&name[1], "align", 5)))
                        return parse_and_apply(&sHAlign, value);
                    break;

                case 'v':
                    if ((name[1] == '\0') || (!strncmp(&name[1], "align", 5)))
                        return parse_and_apply(&sVAlign, value);
                    break;
                default:
                    break;
            }

            return false;
        }

        void TextLayout::reloaded(const tk::StyleSheet *sheet)
        {
            apply_changes();
        }

    } /* namespace ctl */
} /* namespace lsp */

