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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_SWITCHEDPORT_H_
#define LSP_PLUG_IN_PLUG_FW_UI_SWITCHEDPORT_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/ui/IPort.h>
#include <lsp-plug.in/plug-fw/ui/IPortListener.h>

namespace lsp
{
    namespace ui
    {
        class SwitchedPort: public IPort, public IPortListener
        {
            protected:
                enum token_type_t
                {
                    TT_END      = 0,
                    TT_STRING   = 's',
                    TT_INDEX    = 'i'
                };

                #pragma pack(push, 1)
                typedef struct token_t
                {
                    char    type;
                    char    data[];
                } token_t;
                #pragma pack(pop)

            protected:
                IWrapper   *pWrapper;
                size_t      nDimensions;
                IPort     **vControls;
                IPort      *pReference;
                char       *sName;
                token_t    *sTokens;

            protected:
                static token_t     *tokenize(const char *path);
                static token_t     *next_token(token_t *token);
                void                rebind();
                void                destroy();
                inline IPort       *current()
                {
                    if (pReference == NULL)
                        rebind();
                    return pReference;
                };

            public:
                explicit SwitchedPort(IWrapper *wrapper);
                virtual ~SwitchedPort();

            public:
                bool compile(const char *id);
                virtual const char *id() const;

            public:
                virtual void    write(const void *buffer, size_t size);
                virtual void   *buffer();
                virtual float   value();
                virtual float   default_value();
                virtual void    set_value(float value);
                virtual void    notify_all();
                virtual void    notify(IPort *port);
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UI_SWITCHEDPORT_H_ */
