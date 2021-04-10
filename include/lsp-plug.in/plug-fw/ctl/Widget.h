/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_WIDGET_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_WIDGET_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl/types.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Basic widget controller
         */
        class Widget: public ui::IPortListener
        {
            public:
                static const ctl_class_t metadata;

            protected:
                const ctl_class_t  *pClass;
                ui::IWrapper       *pRegistry;
                tk::Widget         *pWidget;

//                CtlColor        sBgColor;
//
//                CtlExpression   sVisibility;
//                CtlExpression   sBright;
//                char           *pVisibilityID;
//                ssize_t         nVisible;
//                ssize_t         nVisibilityKey;
//                bool            bVisibilitySet;
//                bool            bVisibilityKeySet;
//
//                ssize_t         nMinWidth;
//                ssize_t         nMinHeight;

//            protected:
//                void            init_color(color_t value, Color *color);
//                void            init_color(color_t value, LSPColor *color);
//                void            set_lc_attr(widget_attribute_t att, LSPLocalString *s, const char *name, const char *value);

            public:
                explicit Widget(ui::IWrapper *src, tk::Widget *widget);
                virtual ~Widget();

                /** Initialize widget controller
                 *
                 */
                virtual status_t init();

                /** Destroy widget controller
                 *
                 */
                virtual void destroy();

            public:
                /** Get widget
                 *
                 * @return widget
                 */
                virtual tk::Widget  *widget();

                /** Set attribute to widget controller
                 *
                 * @param name attribute name
                 * @param value attribute value
                 */
                virtual void        set(const char *name, const char *value);

//                /** Set attribute to widget controller
//                 *
//                 * @param att attribute identifier
//                 * @param value attribute value
//                 */
//                virtual void set(widget_attribute_t att, const char *value);

                /** Begin internal part of controller
                 *
                 */
                virtual void        begin();

                /** Add child widget
                 *
                 * @param child child widget to add
                 */
                virtual status_t    add(ctl::Widget *child);

                /** End internal part of controller
                 *
                 */
                virtual void        end();

                /** Notify controller about one of port bindings has changed
                 *
                 * @param port port triggered change
                 */
                virtual void        notify(ui::IPort *port);

                /**
                 * Resolve widget by it's unique identifier
                 * @param uid unique widget identifier
                 * @return pointer to resolved widget or NULL
                 */
                virtual tk::Widget *resolve(const char *uid);

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

                /** Get pointer to self as pointer to ctl::Widget class
                 *
                 * @return pointer to self
                 */
                inline ctl::Widget *self()              { return this;  }
        };

        template <class Target>
            inline Target *ctl_cast(ctl::Widget *src)
            {
                return ((src != NULL) && (src->instance_of(&Target::metadata))) ? static_cast<Target *>(src) : NULL;
            }

        template <class Target>
            inline const Target *ctl_cast(const ctl::Widget *src)
            {
                return ((src != NULL) && (src->instance_of(&Target::metadata))) ? static_cast<const Target *>(src) : NULL;
            }

        template <class Target>
            inline Target *ctl_ptrcast(void *src)
            {
                ctl::Widget *w = (src != NULL) ? static_cast<Target *>(src) : NULL;
                return ((w != NULL) && (w->instance_of(&Target::metadata))) ? static_cast<Target *>(w) : NULL;
            }

        template <class Target>
            inline const Target *ctl_ptrcast(const void *src)
            {
                const ctl::Widget *w = (src != NULL) ? static_cast<const Target *>(src) : NULL;
                return ((w != NULL) && (w->instance_of(&Target::metadata))) ? static_cast<const Target *>(w) : NULL;
            }

    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_WIDGET_H_ */
