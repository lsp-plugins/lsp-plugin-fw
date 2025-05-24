/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
    } /* namespace ui */

    namespace ctl
    {
        /**
         * Basic widget controller
         */
        class Widget: public ctl::DOMController, public ui::IPortListener, public ui::ISchemaListener
        {
            public:
                static const ctl_class_t metadata;

            protected:
                class PropListener: public tk::prop::Listener
                {
                    protected:
                        Widget     *pWidget;

                    public:
                        inline PropListener(Widget *w)          { pWidget = w; }
                        virtual void notify(tk::Property *prop);
                        inline void unbind()                    { pWidget = NULL; }
                };

            protected:
                tk::Widget         *wWidget;

                ctl::Boolean        sActivity;
                ctl::Color          sBgColor;
                ctl::Color          sInactiveBgColor;
                ctl::Boolean        sBgInherit;
                ctl::Padding        sPadding;
                ctl::Boolean        sVisibility;
                ctl::Float          sBrightness;
                ctl::Float          sInactiveBrightness;
                ctl::Float          sBgBrightness;
                ctl::Float          sInactiveBgBrightness;
                ctl::Enum           sPointer;

                PropListener        sProperties;        // Properties listener

            protected:
                static const char  *match_prefix(const char *prefix, const char *name);

                static bool         set_allocation(tk::Allocation *alloc, const char *name, const char *value);
                static bool         set_constraints(tk::SizeConstraints *c, const char *name, const char *value);
                static bool         set_layout(tk::Layout *l, const char *param, const char *name, const char *value);
                static bool         set_arrangement(tk::Arrangement *a, const char *param, const char *name, const char *value);
                static bool         set_alignment(tk::Alignment *a, const char *param, const char *name, const char *value);
                static bool         set_text_layout(tk::TextLayout *l, const char *name, const char *value);
                static bool         set_text_layout(tk::TextLayout *l, const char *param, const char *name, const char *value);
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

            protected:
                void                do_destroy();
                bool                bind_port(ui::IPort **port, const char *param, const char *name, const char *value);
                bool                link_port(ui::IPort **port, const char *id);

                virtual void        property_changed(tk::Property *prop);

            public:
                explicit Widget(ui::IWrapper *wrapper, tk::Widget *widget);
                Widget(const Widget &) = delete;
                Widget(Widget &&) = delete;
                virtual ~Widget() override;

                Widget &operator = (const Widget &) = delete;
                Widget &operator = (Widget &&) = delete;

                /**
                 * Initialize widget controller
                 */
                virtual status_t    init() override;

                /**
                 * Destroy widget controller
                 */
                virtual void        destroy() override;

            public: // ctl::DOMController
                /** Set attribute to widget controller
                 *
                 * @param ctx context
                 * @param name attribute name
                 * @param value attribute value
                 */
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;

            public:
                /**
                 * Get widget
                 * @return widget
                 */
                virtual tk::Widget  *widget();

                /**
                 * Add child widget
                 * @param child child widget to add
                 */
                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child);

                /**
                 * Notify controller about one of port bindings has changed
                 * @param port port triggered change
                 * @param flags port modification flags @see notify_flags_t
                 */
                virtual void        notify(ui::IPort *port, size_t flags);

                /**
                 * This method is called when the visual schema has been reloaded
                 */
                virtual void        reloaded(const tk::StyleSheet *sheet);
        };

    } /* namespace ctl */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_WIDGET_H_ */
