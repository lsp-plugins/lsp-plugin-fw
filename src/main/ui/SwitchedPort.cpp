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

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/io/OutMemoryStream.h>

namespace lsp
{
    namespace ui
    {
        SwitchedPort::SwitchedPort(IWrapper *wrapper): IPort(NULL)
        {
            pWrapper        = wrapper;
            nDimensions     = 0;
            vControls       = NULL;
            pReference      = NULL;
            sName           = NULL;
            sTokens         = NULL;
        }

        SwitchedPort::~SwitchedPort()
        {
            destroy();
        }

        SwitchedPort::token_t *SwitchedPort::tokenize(const char *path)
        {
            io::OutMemoryStream os;

            while (path != NULL)
            {
                char c = *path;
                if (c == '\0')
                {
                    if (!os.writeb(TT_END))
                        return NULL;
                    return reinterpret_cast<token_t *>(os.release());
                }

                if (c == '[')
                {
                    path    ++;
                    const char *end   = strchr(path, ']');
                    if (end == NULL)
                        return NULL;

                    if ((os.writeb(TT_INDEX)) < 0)
                        break;
                    if ((os.write(path, end - path)) < 0)
                        break;
                    if ((os.writeb('\0')) < 0)
                        break;
                    path    = end + 1;
                }
                else
                {
                    const char *end   = strchr(path + 1, '[');
                    if (end == NULL)
                        end         = path + strlen(path);

                    if ((os.writeb(TT_STRING)) < 0)
                        break;
                    if ((os.write(path, end - path)) < 0)
                        break;
                    if ((os.writeb('\0')) < 0)
                        break;
                    path    = end;
                }
            }

            return NULL;
        }

        SwitchedPort::token_t *SwitchedPort::next_token(token_t *token)
        {
            if (token == NULL)
                return NULL;
            size_t len  = strlen(token->data);
            return reinterpret_cast<token_t *>(reinterpret_cast<uint8_t *>(token) + len + 2);
        }

        void SwitchedPort::rebind()
        {
            // Unbind from referenced ports
            if (pReference != NULL)
            {
                if (ui::IPort::editing())
                    pReference->end_edit();

                pReference->unbind(this);
                pMetadata       = NULL;
            }

            // Initialize buffer
            LSPString id;

            // Generate port name
            size_t ctl_id   = 0;
            token_t *tok    = sTokens;
            while (tok->type != TT_END)
            {
                if (tok->type == TT_INDEX)
                {
                    int index   = (vControls[ctl_id]) ? vControls[ctl_id]->value() : 0;
                    if (!id.fmt_append_ascii("_%d", index))
                        return;

                    ctl_id      ++;
                }
                else if (tok->type == TT_STRING)
                {
                    if (!id.append_ascii(tok->data))
                        return;
                }
                else
                    break;
                tok     = next_token(tok);
            }

            // Now fetch port by name
            pReference  = pWrapper->port(id.get_ascii());
            if (pReference != NULL)
            {
                pMetadata       = pReference->metadata();
                pReference->bind(this);

                if (ui::IPort::editing())
                    pReference->begin_edit();
            }
        }

        void SwitchedPort::destroy()
        {
            if (pReference != NULL)
            {
                pReference->unbind(this);
                pReference  = NULL;
            }
            if (vControls != NULL)
            {
                // Unbind
                for (size_t i=0; i<nDimensions; ++i)
                {
                    if (vControls[i] != NULL)
                        vControls[i]->unbind(this);
                }

                delete [] vControls;
                vControls = NULL;
            }
            if (sName != NULL)
            {
                free(sName);
                sName       = NULL;
            }
            if (sTokens != NULL)
            {
                free(sTokens);
                sTokens     = NULL;
            }
            pMetadata       = NULL;
        }

        bool SwitchedPort::compile(const char *id)
        {
            destroy();

            sTokens = tokenize(id);
            if (sTokens != NULL)
            {
                sName   = strdup(id);
                if (sName != NULL)
                {
                    // Calculate number of control ports
                    nDimensions     = 0;
                    token_t *tok    = sTokens;
                    while (tok->type != TT_END)
                    {
                        if (tok->type == TT_INDEX)
                            nDimensions++;
                        tok     = next_token(tok);
                    }

                    // Bind control ports
                    vControls       = new IPort *[nDimensions];
                    if (vControls != NULL)
                    {
                        size_t index    = 0;
                        tok             = sTokens;
                        while (tok->type != TT_END)
                        {
                            if (tok->type == TT_INDEX)
                            {
                                IPort *sw         = pWrapper->port(tok->data);
                                if (sw != NULL)
                                    sw->bind(this);
                                vControls[index++]  = sw;
                            }
                            tok     = next_token(tok);
                        }

                        rebind();
                        return true;
                    }
                }
            }

            destroy();
            return false;
        }

        void SwitchedPort::write(const void *buffer, size_t size)
        {
            IPort *p  = current();
            if (p != NULL)
                p->write(buffer, size);
        }

        void *SwitchedPort::buffer()
        {
            IPort *p  = current();
            return (p != NULL) ? p->buffer() : NULL;
        }

        float SwitchedPort::value()
        {
            IPort *p  = current();
            return (p != NULL) ? p->value() : 0.0f;
        }

        float SwitchedPort::default_value()
        {
            IPort *p  = current();
            return (p != NULL) ? p->default_value() : 0.0f;
        }

        void SwitchedPort::set_value(float value)
        {
            IPort *p  = current();
            if (p != NULL)
                p->set_value(value);
        }

        void SwitchedPort::notify_all(size_t flags)
        {
            IPort *p  = current();
            if (p != NULL)
                p->notify_all(flags); // We will receive notify() as subscribers
            else
                IPort::notify_all(flags);
        }

        const char *SwitchedPort::id() const
        {
            return sName;
        }

        void SwitchedPort::notify(IPort *port, size_t flags)
        {
            // Check that event is not from dimension-control port
            for (size_t i=0; i<nDimensions; ++i)
            {
                if (port == vControls[i])
                {
                    rebind();
                    notify_all(flags);
                    return;
                }
            }

            // Proxy notify() event only for active port
            IPort *p  = current();
            if ((p == NULL) || (port != p))
                return;

            // Notify all subscribers
            IPort::notify_all(flags);
        }

        bool SwitchedPort::begin_edit()
        {
            if (!ui::IPort::begin_edit())
                return false;

            ui::IPort *p  = current();
            if (p != NULL)
                p->begin_edit();
            return true;
        }

        bool SwitchedPort::end_edit()
        {
            if (!ui::IPort::end_edit())
                return false;

            ui::IPort *p  = current();
            if (p != NULL)
                p->end_edit();
            return true;
        }

    } /* namespace ctl */
} /* namespace lsp */


