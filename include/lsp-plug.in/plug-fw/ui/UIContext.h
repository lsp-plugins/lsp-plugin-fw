/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_UI_UICONTEXT_H_
#define LSP_PLUG_IN_UI_UICONTEXT_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui/IWrapper.h>
#include <lsp-plug.in/plug-fw/ui/Module.h>
#include <lsp-plug.in/plug-fw/ctl.h>

#include <lsp-plug.in/expr/Expression.h>
#include <lsp-plug.in/expr/Variables.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        class Controller;
        class DOMController;
        class Widget;
        class Overlay;
        class Registry;
    }

    namespace ui
    {
        /**
         * UI context, stores main variables and state of XML document parsing structure
         * which allows to build the UI from XML document
         */
        class UIContext
        {
            protected:
                ui::IWrapper                   *pWrapper;
                ctl::Registry                  *pControllers;
                tk::Registry                   *pWidgets;
                expr::Resolver                 *pResolver;
                lltl::parray<expr::Variables>   vStack;
                lltl::parray<ctl::Overlay>      vOverlays;
                expr::Variables                 vRoot;
                UIOverrides                     sOverrides;

            public:
                explicit UIContext(ui::IWrapper *wrapper, ctl::Registry *controllers, tk::Registry *widgets);
                UIContext(const UIContext &) = delete;
                UIContext(UIContext &&) = delete;
                ~UIContext();

                UIContext & operator = (const UIContext &) = delete;
                UIContext & operator = (UIContext &&) = delete;

                status_t                init();

            public:
                /**
                 * Get the pointer to plugin UI
                 * @return pointer to plugin UI
                 */
                inline ui::Module      *ui()                { return pWrapper->ui();    }

                /**
                 * Get the pointer to the UI wrapper
                 * @return pointer to the UI wrapper
                 */
                inline ui::IWrapper    *wrapper()           { return pWrapper;          }

                /**
                 * Get the registry for controllers
                 * @return the registry for controllers
                 */
                inline ctl::Registry   *controllers()       { return pControllers;      }

                /**
                 * Get the registry for widget
                 * @return the registry for widgets
                 */
                inline tk::Registry    *widgets()           { return pWidgets;          }

                /**
                 * Get list of overlay widget controllers
                 * @return list of overlay widget controllers
                 */
                lltl::parray<ctl::Overlay> *overlays()      { return &vOverlays;        }

                /**
                 * Get the display
                 * @return display
                 */
                inline tk::Display     *display()           { return (pWrapper != NULL) ? pWrapper->ui()->display() : NULL; }

                /**
                 * Get the attribute overrides settings
                 * @return attribute overrides settings
                 */
                inline UIOverrides     *overrides()         { return &sOverrides;       }

            public:
                /**
                 * Start new nested variable scope
                 * @return status of operation
                 */
                status_t                push_scope();

                /**
                 * Remove nested variable scope and destroy all nested variables
                 * @return status of operation
                 */
                status_t                pop_scope();

                /**
                 * Get current variable resolver
                 * @return current variable resolver
                 */
                inline expr::Resolver   *resolver()
                {
                    expr::Variables *r = vStack.last();
                    return (r != NULL) ? r : &vRoot;
                }

                /**
                 * Get current variable scope
                 * @return current variable scope
                 */
                inline expr::Variables *vars()
                {
                    expr::Variables *r = vStack.last();
                    return (r != NULL) ? r : &vRoot;
                }

                /**
                 * Get root variable scope
                 * @return root variable scope
                 */
                inline expr::Variables *root()          { return &vRoot; }

                /**
                 * Evaluate expression
                 * @param eval expression to evaluate
                 * @param expr expression to evaluate
                 * @param flags expression compilation flags
                 * @return status of operation
                 */
                status_t    evaluate(expr::Expression *eval, const LSPString *expr, size_t flags);

                /**
                 * Evaluate expression
                 * @param value value to return
                 * @param expr expression to evaluate
                 * @param flags expression compilation flags
                 * @return status of operation
                 */
                status_t    evaluate(expr::value_t *value, const LSPString *expr, size_t flags);

                /**
                 * Evaluate value and return as string
                 * @param var pointer to store string
                 * @param expr expression
                 * @return status of operation
                 */
                status_t    eval_string(LSPString *value, const LSPString *expr);

                /**
                 * Evaluate value and return as boolean
                 * @param var pointer to store string
                 * @param expr expression
                 * @return status of operation
                 */
                status_t    eval_bool(bool *value, const LSPString *expr);

                /**
                 * Evaluate value and return as integer
                 * @param var pointer to store integer value
                 * @param expr expression
                 * @return status of operation
                 */
                status_t    eval_int(ssize_t *value, const LSPString *expr);

                /**
                 * Create widget controller by the controller's tag name
                 *
                 * @param name the tag name of the widget
                 * @return pointer to widget controller
                 */
                ctl::Controller *create_controller(const LSPString *name);

                /**
                 * Set attributes to controller
                 * @param widget widget to set attributes
                 * @param atts attributes to set
                 * @return status of operation
                 */
                status_t    set_attributes(ctl::DOMController *ctl, const LSPString * const *atts);
        };

    } /* namespace ui */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_UI_UICONTEXT_H_ */
