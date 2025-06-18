/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 9 июн. 2025 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/dsp-units/util/ADSREnvelope.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(AudioEnvelope)
            status_t res;

            if (!name->equals_ascii("aenvelope"))
                return STATUS_NOT_FOUND;

            tk::AudioEnvelope *w = new tk::AudioEnvelope(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::AudioEnvelope *wc  = new ctl::AudioEnvelope(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(AudioEnvelope)

        //-----------------------------------------------------------------
        const ctl_class_t AudioEnvelope::metadata       = { "AudioEnvelope", &Widget::metadata };

        AudioEnvelope::AudioEnvelope(ui::IWrapper *wrapper, tk::AudioEnvelope *widget): ctl::Widget(wrapper, widget)
        {
            pClass              = &metadata;

            for (size_t i=0; i<P_TOTAL; ++i)
            {
                for (size_t j=0; j<R_TOTAL; ++j)
                {
                    point_t *p      = &vPoints[i][j];

                    p->pPort        = NULL;
                    p->pValue       = NULL;
                    p->fOldValue    = 0.0f;
                    p->fNewValue    = 0.0f;
                }

                vTypes[i]           = NULL;
            }

            bCommitting         = false;
            bSubmitting         = false;
        }

        AudioEnvelope::~AudioEnvelope()
        {
        }

        status_t AudioEnvelope::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::AudioEnvelope *ae = tk::widget_cast<tk::AudioEnvelope>(wWidget);
            if (ae != NULL)
            {
                vPoints[P_ATTACK][R_TIME].pValue    = ae->attack_time();
                vPoints[P_ATTACK][R_CURVE].pValue   = ae->attack_curvature();
                vPoints[P_HOLD][R_TIME].pValue      = ae->hold_time();
                vPoints[P_DECAY][R_TIME].pValue     = ae->decay_time();
                vPoints[P_DECAY][R_CURVE].pValue    = ae->decay_curvature();
                vPoints[P_BREAK][R_LEVEL].pValue    = ae->break_level();
                vPoints[P_SLOPE][R_TIME].pValue     = ae->slope_time();
                vPoints[P_SLOPE][R_CURVE].pValue    = ae->slope_curvature();
                vPoints[P_SUSTAIN][R_LEVEL].pValue  = ae->sustain_level();
                vPoints[P_RELEASE][R_TIME].pValue   = ae->release_time();
                vPoints[P_RELEASE][R_CURVE].pValue  = ae->release_curvature();

                sHoldEnabled.init(pWrapper, ae->hold_enabled());
                sBreakEnabled.init(pWrapper, ae->break_enabled());
                sQuadPoint.init(pWrapper, ae->quad_point());
                sFill.init(pWrapper, ae->fill());
                sEditable.init(pWrapper, ae->editable());
                sLineWidth.init(pWrapper, ae->line_width());
                sLineColor.init(pWrapper, ae->line_color());
                sFillColor.init(pWrapper, ae->fill_color());
                sPointSize.init(pWrapper, ae->point_size());
                sPointColor.init(pWrapper, ae->point_color());
                sPointHoverColor.init(pWrapper, ae->point_hover_color());
                sColor.init(pWrapper, ae->color());
                sBorderFlat.init(pWrapper, ae->border_flat());
                sGlass.init(pWrapper, ae->glass());
                sGlassColor.init(pWrapper, ae->glass_color());
                sIPadding.init(pWrapper, ae->ipadding());
                sBorderColor.init(pWrapper, ae->border_color());

                ae->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
                ae->set_curve_function(curve_function, this);
            }

            return STATUS_OK;
        }

        void AudioEnvelope::destroy()
        {
            ctl::Widget::destroy();
        }

        void AudioEnvelope::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::AudioEnvelope *ae = tk::widget_cast<tk::AudioEnvelope>(wWidget);
            if (ae != NULL)
            {
                bind_port(&vPoints[P_ATTACK][R_TIME].pPort, "attack.time.id", name, value);
                bind_port(&vPoints[P_ATTACK][R_CURVE].pPort, "attack.curve.id", name, value);
                bind_port(&vPoints[P_HOLD][R_TIME].pPort, "hold.time.id", name, value);
                bind_port(&vPoints[P_DECAY][R_TIME].pPort, "decay.time.id", name, value);
                bind_port(&vPoints[P_DECAY][R_CURVE].pPort, "decay.curve.id", name, value);
                bind_port(&vPoints[P_BREAK][R_LEVEL].pPort, "break.level.id", name, value);
                bind_port(&vPoints[P_SLOPE][R_TIME].pPort, "slope.time.id", name, value);
                bind_port(&vPoints[P_SLOPE][R_CURVE].pPort, "slope.curve.id", name, value);
                bind_port(&vPoints[P_SUSTAIN][R_LEVEL].pPort, "sustain.level.id", name, value);
                bind_port(&vPoints[P_RELEASE][R_TIME].pPort, "release.time.id", name, value);
                bind_port(&vPoints[P_RELEASE][R_CURVE].pPort, "release.curve.id", name, value);

                bind_port(&vTypes[P_ATTACK], "attack.type.id", name, value);
                bind_port(&vTypes[P_DECAY], "decay.type.id", name, value);
                bind_port(&vTypes[P_SLOPE], "slope.type.id", name, value);
                bind_port(&vTypes[P_RELEASE], "release.type.id", name, value);

                sHoldEnabled.set("hold.enabled", name, value);
                sBreakEnabled.set("break.enabled", name, value);
                sQuadPoint.set("point.quad", name, value);
                sFill.set("fill", name, value);
                sEditable.set("editable", name, value);
                sLineWidth.set("line.width", name, value);
                sLineColor.set("line.color", name, value);
                sFillColor.set("fill.color", name, value);
                sPointSize.set("point.size", name, value);
                sPointColor.set("point.color", name, value);
                sPointHoverColor.set("point.hover.color", name, value);
                sColor.set("color", name, value);
                sBorderFlat.set("border.flat", name, value);
                sGlass.set("glass", name, value);
                sGlassColor.set("glass.color", name, value);
                sBorderColor.set("border.color", name, value);

                set_constraints(ae->constraints(), name, value);
            }

            return ctl::Widget::set(ctx, name, value);
        }

        float AudioEnvelope::get_normalized(ui::IPort *port)
        {
            if (port == NULL)
                return 0.0f;

            const meta::port_t *meta = port->metadata();
            if (meta == NULL)
                return 0.0f;

            if ((meta->flags & (meta::F_UPPER | meta::F_LOWER)) != (meta::F_UPPER | meta::F_LOWER))
                return 0.0f;

            const float value = port->value();
            return (value - meta->min) / (meta->max - meta->min);
        }

        void AudioEnvelope::set_normalized(ui::IPort *port, float value)
        {
            if (port == NULL)
                return;

            const meta::port_t *meta = port->metadata();
            if (meta == NULL)
                return;

            if ((meta->flags & (meta::F_UPPER | meta::F_LOWER)) != (meta::F_UPPER | meta::F_LOWER))
                return;

            port->set_value(meta->min + value * (meta->max - meta->min));
        }

        void AudioEnvelope::end(ui::UIContext *ctx)
        {
            // Synchronize port values
            for (size_t i=0; i<P_TOTAL; ++i)
                for (size_t j=0; j<R_TOTAL; ++j)
                {
                    point_t *p      = &vPoints[i][j];
                    if ((p->pPort == NULL) || (p->pValue == NULL))
                        continue;

                    const float value = get_normalized(p->pPort);
                    p->fOldValue    = value;
                    p->fNewValue    = value;
                }

            // Align time points
            arrange_time_values();
            commit_values();

            ctl::Widget::end(ctx);
        }

        void AudioEnvelope::arrange_time_values()
        {
            point_t *prev   = NULL;
            for (size_t i=0; i<P_TOTAL; ++i)
            {
                point_t *p      = &vPoints[i][R_TIME];
                if (p->pPort == NULL)
                    continue;

                p->fNewValue    = lsp_limit(p->fNewValue, 0.0f, 1.0f);
                if ((prev != NULL) && (p->fNewValue < prev->fNewValue))
                    p->fNewValue    = prev->fNewValue;

                prev            = p;
            }
        }

        void AudioEnvelope::commit_values()
        {
            // Avoid recursive calls
            if (bCommitting)
                return;
            bCommitting = true;
            lsp_finally { bCommitting = false; };

            // Update values on the widget
            for (size_t i=0; i<P_TOTAL; ++i)
                for (size_t j=0; j<R_TOTAL; ++j)
                {
                    point_t *p      = &vPoints[i][j];
                    if ((p->pPort == NULL) || (p->pValue == NULL))
                        continue;

                    if (p->fNewValue != p->pValue->get())
                        p->pValue->set(p->fNewValue);
                }
        }

        void AudioEnvelope::sync_time_values(point_t *actor)
        {
            for (size_t i=0; i<P_TOTAL; ++i)
            {
                point_t *dst    = &vPoints[i][R_TIME];
                if (dst->pPort == NULL)
                    continue;

                if ((dst < actor) && (dst->fNewValue > actor->fNewValue))
                    dst->fNewValue      = actor->fNewValue;
                else if ((dst > actor) && (dst->fNewValue < actor->fNewValue))
                    dst->fNewValue      = actor->fNewValue;
            }
        }

        void AudioEnvelope::notify(ui::IPort *port, size_t flags)
        {
            // Update state of points
            for (size_t i=0; i<P_TOTAL; ++i)
            {
                for (size_t j=0; j<R_TOTAL; ++j)
                {
                    point_t *p      = &vPoints[i][j];
                    if ((p->pPort != port) || (p->pValue == NULL))
                        continue;

                    p->fNewValue    = get_normalized(p->pPort);
                    if (j == R_TIME)
                        sync_time_values(p);
                }

                // Query curve redraw if curve segment type port has changed
                if ((vTypes[i] != NULL) && (vTypes[i] == port))
                    wWidget->query_draw();
            }

            // Commit changes for points
            commit_values();
        }

        void AudioEnvelope::submit_ports()
        {
            // Avoid recursive calls
            if (bSubmitting)
                return;
            bSubmitting = true;
            lsp_finally { bSubmitting = false; };

            // Update port values
            for (size_t i=0; i<P_TOTAL; ++i)
                for (size_t j=0; j<R_TOTAL; ++j)
                {
                    point_t *p      = &vPoints[i][j];
                    if ((p->pPort == NULL) || (p->pValue == NULL))
                        continue;

                    p->fNewValue = p->pValue->get();
                    if (p->fNewValue != p->fOldValue)
                        set_normalized(p->pPort, p->fNewValue);
                }

            // Commit new values and notify about changes
            for (size_t i=0; i<P_TOTAL; ++i)
                for (size_t j=0; j<R_TOTAL; ++j)
                {
                    point_t *p      = &vPoints[i][j];
                    if ((p->pPort == NULL) || (p->pValue == NULL))
                        continue;

                    if (p->fNewValue != p->fOldValue)
                    {
                        const float value   = get_normalized(p->pPort);
                        p->fNewValue        = value;
                        p->fOldValue        = value;

                        p->pPort->notify_all(ui::PORT_USER_EDIT);
                    }
                }
        }

        size_t AudioEnvelope::get_function(points_t point)
        {
            ui::IPort *port = vTypes[point];
            return (port != NULL) ? port->value() : dspu::ADSREnvelope::ADSR_NONE;
        }

        void AudioEnvelope::curve_function(float *y, const float *x, size_t count, const tk::AudioEnvelope *sender, void *data)
        {
            if (sender == NULL)
                return;

            AudioEnvelope *self = static_cast<AudioEnvelope *>(data);
            if (self == NULL)
                return;

            dspu::ADSREnvelope e;

            e.set_attack(
                sender->attack_time()->get(),
                sender->attack_curvature()->get(),
                dspu::ADSREnvelope::function_t(self->get_function(P_ATTACK)));
            e.set_hold(
                sender->hold_time()->get(),
                sender->hold_enabled()->get());
            e.set_decay(
                sender->decay_time()->get(),
                sender->decay_curvature()->get(),
                dspu::ADSREnvelope::function_t(self->get_function(P_DECAY)));
            e.set_break(
                sender->break_level()->get(),
                sender->break_enabled()->get());
            e.set_slope(
                sender->slope_time()->get(),
                sender->slope_curvature()->get(),
                dspu::ADSREnvelope::function_t(self->get_function(P_SLOPE)));
            e.set_sustain_level(sender->sustain_level()->get());
            e.set_release(
                sender->release_time()->get(),
                sender->release_curvature()->get(),
                dspu::ADSREnvelope::function_t(self->get_function(P_RELEASE)));

            e.process(y, x, count);
        }

        status_t AudioEnvelope::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::AudioEnvelope *self    = static_cast<ctl::AudioEnvelope *>(ptr);
            if (self != NULL)
                self->submit_ports();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */
