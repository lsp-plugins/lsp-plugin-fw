/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 28 сент. 2024 г.
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

#include <lsp-plug.in/io/Dir.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/string.h>

#define ANAV_STYLE_ACTIVE               "AudioNavigator::Active"
#define ANAV_STYLE_INACTIVE             "AudioNavigator::Inactive"

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(AudioNavigator)
            status_t res;

            if (!name->equals_ascii("anavigator"))
                return STATUS_NOT_FOUND;

            tk::Button *w = new tk::Button(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::AudioNavigator *wc = new ctl::AudioNavigator(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(AudioNavigator)

        //---------------------------------------------------------------------
        const ctl_class_t AudioNavigator::metadata = { "AudioNavigator", &Widget::metadata };

        AudioNavigator::AudioNavigator(ui::IWrapper *wrapper, tk::Button *widget):
            Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            bActive         = false;
            enAction        = A_NEXT;
        }

        AudioNavigator::~AudioNavigator()
        {
        }

        status_t AudioNavigator::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn != NULL)
            {
                sColor.init(pWrapper, btn->color());
                sTextColor.init(pWrapper, btn->text_color());
                sBorderColor.init(pWrapper, btn->border_color());
                sHoverColor.init(pWrapper, btn->hover_color());
                sTextHoverColor.init(pWrapper, btn->text_hover_color());
                sBorderHoverColor.init(pWrapper, btn->border_hover_color());
                sHoleColor.init(pWrapper, btn->hole_color());

                sEditable.init(pWrapper, btn->editable());
                sTextPad.init(pWrapper, btn->text_padding());
                sText.init(pWrapper, btn->text());

                // Bind slots
                btn->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
            }

            return STATUS_OK;
        }

        AudioNavigator::action_t AudioNavigator::parse_action(const char *action)
        {
            if ((!strcasecmp(action, "begin")) ||
                (!strcasecmp(action, "start")) ||
                (!strcasecmp(action, "head")) ||
                (!strcasecmp(action, "first")))
                return A_FIRST;
            if ((!strcasecmp(action, "end")) ||
                (!strcasecmp(action, "tail")) ||
                (!strcasecmp(action, "last")))
                return A_LAST;
            if ((!strcasecmp(action, "step")) ||
                (!strcasecmp(action, "forward")) ||
                (!strcasecmp(action, "next")))
                return A_NEXT;
            if ((!strcasecmp(action, "prev")) ||
                (!strcasecmp(action, "previous")) ||
                (!strcasecmp(action, "back")))
                return A_PREVIOUS;
            if ((!strcasecmp(action, "fast_forward")) ||
                (!strcasecmp(action, "ff")) ||
                (!strcasecmp(action, "roll_forward")))
                return A_FAST_FORWARD;
            if ((!strcasecmp(action, "fast_backward")) ||
                (!strcasecmp(action, "fb")) ||
                (!strcasecmp(action, "rewind")) ||
                (!strcasecmp(action, "rew")) ||
                (!strcasecmp(action, "roll_backward")))
                return A_FAST_BACKWARD;

            return A_NONE;
        }

        void AudioNavigator::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sHoverColor.set("hover.color", name, value);
                sHoverColor.set("hcolor", name, value);
                sTextHoverColor.set("text.hover.color", name, value);
                sTextHoverColor.set("thcolor", name, value);
                sBorderHoverColor.set("border.hover.color", name, value);
                sBorderHoverColor.set("bhcolor", name, value);

                sHoleColor.set("hole.color", name, value);

                sEditable.set("editable", name, value);
                sTextPad.set("text.padding", name, value);
                sTextPad.set("text.pad", name, value);
                sTextPad.set("tpadding", name, value);
                sTextPad.set("tpad", name, value);
                sHover.set("hover", name, value);
                sText.set("text", name, value);

                set_font(btn->font(), "font", name, value);
                set_constraints(btn->constraints(), name, value);
                set_param(btn->led(), "led", name, value);
                set_param(btn->hole(), "hole", name, value);
                set_param(btn->flat(), "flat", name, value);
                set_param(btn->text_clip(), "text.clip", name, value);
                set_param(btn->text_adjust(), "text.adjust", name, value);
                set_param(btn->text_clip(), "tclip", name, value);
                set_param(btn->font_scaling(), "font.scaling", name, value);
                set_param(btn->font_scaling(), "font.scale", name, value);
                set_param(btn->mode(), "mode", name, value);
                set_text_layout(btn->text_layout(), name, value);

                if (!strcmp(name, "action"))
                    enAction        = parse_action(value);

                sDirController.set(name, value);
            }

            Widget::set(ctx, name, value);
        }

        void AudioNavigator::set_activity(bool active)
        {
            if (bActive == active)
                return;

            bActive     = active;
            update_styles();
        }

        void AudioNavigator::update_styles()
        {
            if (wWidget == NULL)
                return;

            revoke_style(wWidget, ANAV_STYLE_ACTIVE);
            revoke_style(wWidget, ANAV_STYLE_INACTIVE);
            inject_style(wWidget, (bActive) ? ANAV_STYLE_ACTIVE : ANAV_STYLE_INACTIVE);
        }

        void AudioNavigator::sync_state()
        {
            // Check action type
            if (enAction == A_NONE)
                return set_activity(false);

            // Check that port points to the valid data
            const meta::port_t *meta = (pPort != NULL) ? pPort->metadata() : NULL;
            if ((meta == NULL) || (!meta::is_path_port(meta)))
                return set_activity(false);

            // Obtain current file name
            const char *path = pPort->buffer<const char>();
            if ((path == NULL) || (strlen(path) == 0))
                return set_activity(false);

            // Apply changes to the controller
            sDirController.set_current_file(path);
            set_activity(sDirController.valid());
        }

        void AudioNavigator::end(ui::UIContext *ctx)
        {
            update_styles();
            sync_state();
            Widget::end(ctx);
        }

        void AudioNavigator::apply_action()
        {
            if ((!bActive) || (pPort == NULL))
                return;

            ssize_t new_index       = sDirController.file_index();
            const ssize_t files     = sDirController.num_files();
            switch (enAction)
            {
                case A_FIRST:
                    new_index       = 0;
                    break;
                case A_LAST:
                    new_index       = files - 1;
                    break;
                case A_NEXT:
                    new_index       = (lsp_max(new_index, 0) + 1) % files;
                    break;
                case A_PREVIOUS:
                    new_index       = (lsp_max(new_index, 0) - 1) % files;
                    if (new_index < 0)
                        new_index      += files;
                    break;
                case A_FAST_FORWARD:
                    new_index       = (lsp_max(new_index, 0) + 10) % files;
                    break;
                case A_FAST_BACKWARD:
                    new_index       = (lsp_max(new_index, 0) - 10) % files;
                    if (new_index < 0)
                        new_index      += files;
                    break;

                case A_NONE:
                default:
                    return;
            }

            // Ensure that file index has changed
            if (new_index == sDirController.file_index())
                return;

            io::Path file;
            if (file.set(sDirController.directory(), sDirController.files()->uget(new_index)) != STATUS_OK)
                return;
            const char *buf = file.as_utf8();
            if (buf == NULL)
                return;

            // Apply changes
            pPort->write(buf, strlen(buf));
            pPort->notify_all(ui::PORT_USER_EDIT);
        }

        void AudioNavigator::notify(ui::IPort *port, size_t flags)
        {
            if ((pPort != NULL) && (port == pPort))
                sync_state();
        }

        status_t AudioNavigator::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::AudioNavigator *self   = static_cast<ctl::AudioNavigator *>(ptr);
            if (self != NULL)
                self->apply_action();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */





