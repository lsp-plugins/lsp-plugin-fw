/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 мая 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_CONTROLLER_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_CONTROLLER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ui
    {
        class UIContext;
    } /* namespace ui */

    namespace ctl
    {
        class Expression;

        /**
         * Controller
         */
        class Controller: public ui::IPortListener
        {
            public:
                static const ctl_class_t metadata;

            protected:
                const ctl_class_t  *pClass;
                ui::IWrapper       *pWrapper;

            protected:
                static bool         set_expr(ctl::Expression *expr, const char *param, const char *name, const char *value);
                static bool         set_value(bool *v, const char *param, const char *name, const char *value);
                static bool         set_value(ssize_t *v, const char *param, const char *name, const char *value);
                static bool         set_value(size_t *v, const char *param, const char *name, const char *value);
                static bool         set_value(float *v, const char *param, const char *name, const char *value);
                static bool         set_value(LSPString *v, const char *param, const char *name, const char *value);

                bool                bind_port(ui::IPort **port, const char *param, const char *name, const char *value);
                bool                link_port(ui::IPort **port, const char *id);

            public:
                explicit Controller(ui::IWrapper *wrapper);
                Controller(const Controller &) = delete;
                Controller(Controller &&) = delete;
                virtual ~Controller();

                Controller & operator = (const Controller &) = delete;
                Controller & operator = (Controller &&) = delete;

                /** Initialize widget controller
                 *
                 */
                virtual status_t    init();

                /** Destroy widget controller
                 *
                 */
                virtual void        destroy();

            public:
                /** Get pointer to self as pointer to ctl::Widget class
                 *
                 * @return pointer to self
                 */
                inline ctl::Controller *self()              { return this;  }

            //---------------------------------------------------------------------------------
            // Metadata, casting and type information
            public:
                /** Get widget controller class
                 *
                 * @return actual widget controller class metadata
                 */
                inline const ctl_class_t *get_class() const { return pClass; }

                /** Check wheter the widget is instance of some class
                 *
                 * @param wclass widget class
                 * @return true if widget is instance of some class
                 */
                bool                instance_of(const ctl_class_t *wclass) const;
                inline bool         instance_of(const ctl_class_t &wclass) const { return instance_of(&wclass); }

                /** Another way to check if widget is instance of some class
                 *
                 * @return true if widget is instance of some class
                 */
                template <class Target>
                inline bool instance_of() const { return instance_of(&Target::metadata); };

                /** Cast widget to another type
                 *
                 * @return pointer to widget or NULL if cast failed
                 */
                template <class Target>
                inline Target *cast()            { return instance_of(&Target::metadata) ? static_cast<Target *>(this) : NULL; }

                /** Cast widget to another type
                 *
                 * @return pointer to widget or NULL if cast failed
                 */
                template <class Target>
                inline const Target *cast() const { return instance_of(&Target::metadata) ? static_cast<const Target *>(this) : NULL; }
        };

        template <class Target>
        inline Target *ctl_cast(ctl::Controller *src)
        {
            return ((src != NULL) && (src->instance_of(&Target::metadata))) ? static_cast<Target *>(src) : NULL;
        }

        template <class Target>
        inline const Target *ctl_cast(const ctl::Controller *src)
        {
            return ((src != NULL) && (src->instance_of(&Target::metadata))) ? static_cast<const Target *>(src) : NULL;
        }

        template <class Target>
        inline Target *ctl_ptrcast(void *src)
        {
            ctl::Controller *ctl = (src != NULL) ? static_cast<Target *>(src) : NULL;
            return ((ctl != NULL) && (ctl->instance_of(&Target::metadata))) ? static_cast<Target *>(ctl) : NULL;
        }

        template <class Target>
        inline const Target *ctl_ptrcast(const void *src)
        {
            const ctl::Controller *ctl = (src != NULL) ? static_cast<const Target *>(src) : NULL;
            return ((ctl != NULL) && (ctl->instance_of(&Target::metadata))) ? static_cast<const Target *>(ctl) : NULL;
        }

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_CONTROLLER_H_ */
