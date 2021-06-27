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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

#include <lsp-plug.in/plug-fw/ui.h>

#include <lsp-plug.in/plug-fw/ctl/types.h>
#include <lsp-plug.in/plug-fw/ctl/util/Expression.h>

namespace lsp
{
    namespace ui
    {
        class UIContext;
    }

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
                ui::IWrapper       *pWrapper;
                tk::Widget         *wWidget;

                ctl::Color          sBgColor;
                ctl::Boolean        sBgInherit;
                ctl::Padding        sPadding;
                ctl::Boolean        sVisibility;

            protected:
                static const char  *match_prefix(const char *prefix, const char *name);

//                static bool         set_padding(tk::Padding *pad, const char *param, const char *name, const char *value);
                static bool         set_allocation(tk::Allocation *alloc, const char *name, const char *value);
                static bool         set_constraints(tk::SizeConstraints *c, const char *name, const char *value);
                static bool         set_layout(tk::Layout *l, const char *name, const char *value);
                static bool         set_arrangement(tk::Arrangement *a, const char *param, const char *name, const char *value);
                static bool         set_alignment(tk::Alignment *a, const char *param, const char *name, const char *value);
                static bool         set_text_layout(tk::TextLayout *l, const char *name, const char *value);
                static bool         set_text_fitness(tk::TextFitness *l, const char *param, const char *name, const char *value);
                static bool         set_expr(ctl::Expression *expr, const char *param, const char *name, const char *value);
                static bool         set_font(tk::Font *f, const char *param, const char *name, const char *value);
                static bool         set_size_range(tk::SizeRange *r, const char *param, const char *name, const char *value);
                static bool         set_param(tk::Boolean *b, const char *param, const char *name, const char *value);
                static bool         set_param(tk::Integer *i, const char *param, const char *name, const char *value);
                static bool         set_param(tk::Float *f, const char *param, const char *name, const char *value);
                static bool         set_param(tk::Enum *en, const char *param, const char *name, const char *value);
                static bool         set_embedding(tk::Embedding *e, const char *name, const char *value);
                static bool         set_orientation(tk::Orientation *o, const char *name, const char *value);
                static bool         set_value(bool *v, const char *param, const char *name, const char *value);
                static bool         set_value(ssize_t *v, const char *param, const char *name, const char *value);
                static bool         set_value(size_t *v, const char *param, const char *name, const char *value);
                static bool         set_value(float *v, const char *param, const char *name, const char *value);
                static bool         set_value(LSPString *v, const char *param, const char *name, const char *value);

                static void         inject_style(tk::Widget *widget, const char *style_name);
                static void         revoke_style(tk::Widget *widget, const char *style_name);
                static void         add_parent_style(tk::Widget *widget, const char *style_name);
                static bool         assign_styles(tk::Widget *widget, const char *style_list, bool remove_parents);

            protected:
                bool                bind_port(ui::IPort **port, const char *param, const char *name, const char *value);

            public:
                explicit Widget(ui::IWrapper *wrapper, tk::Widget *widget);
                virtual ~Widget();

                /** Initialize widget controller
                 *
                 */
                virtual status_t    init();

                /** Destroy widget controller
                 *
                 */
                virtual void        destroy();

            public:
                /** Get widget
                 *
                 * @return widget
                 */
                virtual tk::Widget  *widget();

                /** Set attribute to widget controller
                 *
                 * @param ctx context
                 * @param name attribute name
                 * @param value attribute value
                 */
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);

                /** Begin internal part of controller
                 *
                 */
                virtual void        begin(ui::UIContext *ctx);

                /** Add child widget
                 *
                 * @param child child widget to add
                 */
                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child);

                /** End internal part of controller
                 *
                 */
                virtual void        end(ui::UIContext *ctx);

                /** Notify controller about one of port bindings has changed
                 *
                 * @param port port triggered change
                 */
                virtual void        notify(ui::IPort *port);

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
