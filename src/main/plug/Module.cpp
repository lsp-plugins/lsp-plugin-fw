/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace plug
    {
        Module::Module(const meta::plugin_t *meta)
        {
            lsp_trace("this=%p", this);

            pMetadata       = meta;
            pWrapper        = NULL;
            fSampleRate     = -1;
            nLatency        = 0;
            bActivated      = false;
            bUIActive       = false;
        }

        Module::~Module()
        {
            lsp_trace("this=%p", this);
        }

        void Module::activate_ui()
        {
            if (bUIActive)
                return;

            bUIActive       = true;
            lsp_trace("UI has been activated");
            ui_activated();
        }

        void Module::deactivate_ui()
        {
            if (!bUIActive)
                return;

            bUIActive       = false;
            lsp_trace("UI has been deactivated");
            ui_deactivated();
        }

        void Module::activate()
        {
            if (bActivated)
                return;

            bActivated      = true;
            activated();
            pWrapper->query_display_draw();
        }

        void Module::deactivate()
        {
            if (!bActivated)
                return;

            bActivated      = false;
            deactivated();
            pWrapper->query_display_draw();
        }

        void Module::init(IWrapper *wrapper, IPort **ports)
        {
            pWrapper        = wrapper;
        }

        void Module::set_sample_rate(long sr)
        {
            if (fSampleRate == sr)
                return;

            fSampleRate = sr;
            update_sample_rate(sr);
        };

        void Module::update_sample_rate(long sr)
        {
        }

        void Module::activated()
        {
        }

        void Module::deactivated()
        {
        }

        void Module::ui_activated()
        {
        }

        void Module::ui_deactivated()
        {
        }

        void Module::destroy()
        {
            bActivated      = false;
        }

        void Module::update_settings()
        {
        }

        bool Module::set_position(const position_t *pos)
        {
            return false;
        }

        void Module::process(size_t samples)
        {
        }

        bool Module::inline_display(ICanvas *cv, size_t width, size_t height)
        {
            return false;
        }

        core::KVTStorage *Module::kvt_lock()
        {
            return (pWrapper != NULL) ? pWrapper->kvt_lock() : NULL;
        }

        core::KVTStorage *Module::kvt_trylock()
        {
            return (pWrapper != NULL) ? pWrapper->kvt_trylock() : NULL;
        }

        void Module::kvt_release()
        {
            if (pWrapper != NULL)
                pWrapper->kvt_release();
        }

        void Module::state_saved()
        {
        }

        void Module::state_loaded()
        {
        }

        void Module::dump(dspu::IStateDumper *v) const
        {
        }
    }
}

