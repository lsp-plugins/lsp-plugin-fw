/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_VALUEPORT_H_
#define LSP_PLUG_IN_PLUG_FW_UI_VALUEPORT_H_

namespace lsp
{
    namespace ui
    {
        class ValuePort: public IPort
        {
            private:
                float   fValue;
                float   fPending;

            public:
                explicit ValuePort(const meta::port_t *meta);
                virtual ~ValuePort();

            public:
                void commitValue(float value);

                void sync();

            public:
                virtual float value();
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UI_VALUEPORT_H_ */
