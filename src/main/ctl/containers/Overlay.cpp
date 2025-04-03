/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 апр. 2025 г.
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
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Overlay)
            status_t res;

            if (!name->equals_ascii("overlay"))
                return STATUS_NOT_FOUND;

            tk::Overlay *w = new tk::Overlay(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Overlay *wc  = new ctl::Overlay(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Overlay)

        //-----------------------------------------------------------------
        const ctl_class_t Overlay::metadata     = { "Overlay", &Widget::metadata };

        Overlay::Overlay(ui::IWrapper *wrapper, tk::Overlay *widget):
            Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            fHOrigin        = 0.5f;
            fVOrigin        = 0.5f;
            fHAlign         = 0.0f;
            fVAlign         = 0.0f;
        }

        Overlay::~Overlay()
        {
            tk::Overlay *ov = tk::widget_cast<tk::Overlay>(wWidget);
            if (ov != NULL)
                ov->set_position_function(NULL, NULL);
        }

        status_t Overlay::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Overlay *ov = tk::widget_cast<tk::Overlay>(wWidget);
            if (ov != NULL)
            {
                sTransparency.init(pWrapper, ov->transparency());
                sPriority.init(pWrapper, ov->priority());
                sAutoClose.init(pWrapper, ov->auto_close());
                sBorderRadius.init(pWrapper, ov->border_radius());
                sBorderSize.init(pWrapper, ov->border_size());
                sBorderColor.init(pWrapper, ov->border_color());

                sHOrigin.init(pWrapper, this);
                sVOrigin.init(pWrapper, this);
                sHAlign.init(pWrapper, this);
                sVAlign.init(pWrapper, this);

                sLayoutHAlign.init(pWrapper, this);
                sLayoutVAlign.init(pWrapper, this);
                sLayoutHScale.init(pWrapper, this);
                sLayoutVScale.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void Overlay::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Overlay *ov = tk::widget_cast<tk::Overlay>(wWidget);
            if (ov != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sTransparency.set("transparency", name, value);
                sTransparency.set("alpha", name, value);

                sPriority.set("priority", name, value);

                sAutoClose.set("auto_close", name, value);
                sAutoClose.set("close.auto", name, value);
                sAutoClose.set("aclose", name, value);

                sBorderRadius.set("border.radius", name, value);
                sBorderRadius.set("bradius", name, value);

                sBorderSize.set("border.size", name, value);
                sBorderSize.set("bsize", name, value);

                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);

                set_expr(&sHOrigin, "origin.hpos", name, value);
                set_expr(&sHOrigin, "hpos", name, value);
                set_expr(&sVOrigin, "origin.vpos", name, value);
                set_expr(&sVOrigin, "vpos", name, value);
                set_expr(&sHAlign, "halign", name, value);
                set_expr(&sVAlign, "valign", name, value);

                set_expr(&sLayoutHAlign, "layout.align", name, value);
                set_expr(&sLayoutVAlign, "layout.align", name, value);
                set_expr(&sLayoutHAlign, "layout.halign", name, value);
                set_expr(&sLayoutVAlign, "layout.valign", name, value);

                set_expr(&sLayoutHScale, "layout.scale", name, value);
                set_expr(&sLayoutVScale, "layout.scale", name, value);
                set_expr(&sLayoutHScale, "layout.hscale", name, value);
                set_expr(&sLayoutVScale, "layout.vscale", name, value);

                set_constraints(ov->constraints(), name, value);

                if (!strcmp(name, "trigger"))
                    sTriggerWID.set_utf8(value);
                if (!strcmp(name, "area"))
                    sAreaWID.set_utf8(value);

                ov->set_position_function(calc_position, this);
                ov->slots()->bind(tk::SLOT_HIDE, slot_on_hide, this);
            }

            return Widget::set(ctx, name, value);
        }

        void Overlay::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            // Internal layout
            if ((sLayoutHAlign.depends(port)) ||
                (sLayoutVAlign.depends(port)) ||
                (sLayoutHScale.depends(port)) ||
                (sLayoutVScale.depends(port)))
                update_layout_alignment();

            // Position layout
            if ((sHOrigin.depends(port)) ||
                (sVOrigin.depends(port)) ||
                (sHAlign.depends(port)) ||
                (sVAlign.depends(port)))
                update_alignment();

            // Visibility
            if (port == pPort)
            {
                tk::Overlay *ov = tk::widget_cast<tk::Overlay>(wWidget);
                if (ov != NULL)
                    ov->visibility()->set(pPort->value() >= 0.5f);
            }
        }

        status_t Overlay::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::Overlay *ov = tk::widget_cast<tk::Overlay>(wWidget);
            return (ov != NULL) ? ov->add(child->widget()) : STATUS_BAD_STATE;
        }

        void Overlay::end(ui::UIContext *ctx)
        {
            update_layout_alignment();
            update_alignment();
            Widget::end(ctx);
        }

        void Overlay::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);
            update_layout_alignment();
            update_alignment();
        }

        void Overlay::update_layout_alignment()
        {
            tk::Overlay *ov = tk::widget_cast<tk::Overlay>(wWidget);
            if (ov == NULL)
                return;

            if (sLayoutHAlign.valid())
                ov->layout()->set_halign(sLayoutHAlign.evaluate());
            if (sLayoutVAlign.valid())
                ov->layout()->set_valign(sLayoutVAlign.evaluate());
            if (sLayoutHScale.valid())
                ov->layout()->set_hscale(sLayoutHScale.evaluate());
            if (sLayoutVScale.valid())
                ov->layout()->set_vscale(sLayoutVScale.evaluate());
        }

        bool Overlay::update_float(float & value, ctl::Expression & expr)
        {
            if (!expr.valid())
                return false;
            const float new_value = expr.evaluate();
            if (new_value == value)
                return false;

            value   = new_value;
            return true;
        }

        void Overlay::update_alignment()
        {
            tk::Overlay *ov = tk::widget_cast<tk::Overlay>(wWidget);
            if (ov == NULL)
                return;

            size_t changes = 0;
            if (update_float(fHAlign, sHAlign))
                ++changes;
            if (update_float(fVAlign, sVAlign))
                ++changes;
            if (update_float(fHOrigin, sHOrigin))
                ++changes;
            if (update_float(fVOrigin, sVOrigin))
                ++changes;

            if (changes > 0)
                ov->query_resize();
        }

        bool Overlay::calc_position(ws::rectangle_t *rect, tk::Overlay *overlay, void *data)
        {
            Overlay *self = static_cast<Overlay *>(data);
            return (self != NULL) ? self->calc_position(rect, overlay) : false;
        }

        bool Overlay::calc_position(ws::rectangle_t *rect, tk::Overlay *ov)
        {
            // Get trigger and area rectangles
            ws::rectangle_t trigger, area, pad_area;
            if (!get_area(&trigger, &sTriggerWID))
                return false;
            if (!get_area(&area, &sAreaWID))
                return false;

            // Compute origin
            const ssize_t origin_x  = trigger.nLeft + trigger.nWidth * fHOrigin;
            const ssize_t origin_y  = trigger.nTop + trigger.nHeight * fVOrigin;

            const float scaling     = lsp_max(ov->scaling()->get(), 0.0f);
            ov->padding()->enter(&pad_area, &area, scaling);

            // Compute position
            rect->nLeft             = origin_x + rect->nWidth * fHAlign;
            rect->nTop              = origin_y + rect->nHeight * fVAlign;

            // Constraint position
            rect->nLeft             = lsp_max(rect->nLeft, pad_area.nLeft);
            rect->nTop              = lsp_max(rect->nLeft, pad_area.nTop);
            rect->nLeft            -= lsp_max((rect->nLeft + rect->nWidth) - (pad_area.nLeft + pad_area.nWidth), 0);
            rect->nTop             -= lsp_max((rect->nTop + rect->nHeight) - (pad_area.nTop + pad_area.nHeight), 0);

            return true;
        }

        bool Overlay::get_area(ws::rectangle_t *rect, const LSPString *wid)
        {
            // Check if widget name was passed
            tk::Widget *w = NULL;
            if ((wid != NULL) && (!wid->is_empty()))
                w = pWrapper->controller()->widgets()->find(wid);

            // Fall-back to default widget name
            if (w == NULL)
                w = pWrapper->controller()->widgets()->find("plugin_content");

            // Obtain widget size
            if (w != NULL)
                w->get_rectangle(rect);

            return w != NULL;
        }

        status_t Overlay::slot_on_hide(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Overlay *self      = static_cast<ctl::Overlay *>(ptr);
            if (self != NULL)
                self->on_hide_overlay();
            return STATUS_OK;
        }

        void Overlay::on_hide_overlay()
        {
            if (pPort == NULL)
                return;

            tk::Overlay *ov = tk::widget_cast<tk::Overlay>(wWidget);
            if (ov == NULL)
                return;

            // Reset related port to default value
            if (ov->auto_close()->get())
            {
                pPort->set_default();
                pPort->notify_all(ui::PORT_USER_EDIT);
            }
        }

    } /* namespace ctl */
} /* namespace lsp */


