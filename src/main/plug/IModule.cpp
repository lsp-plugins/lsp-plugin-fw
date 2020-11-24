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
        IModule::IModule(const meta::plugin_t *meta)
        {
            pMetadata       = meta;
            pWrapper        = NULL;
            fSampleRate     = -1;
            nLatency        = 0;
            bActivated      = false;
            bUIActive       = true;
        }

        IModule::~IModule()
        {
        }

        void IModule::activate_ui()
        {
            if (bUIActive)
                return;

            bUIActive       = true;
            lsp_trace("UI has been activated");
            ui_activated();
        }

        void IModule::deactivate_ui()
        {
            if (!bUIActive)
                return;

            bUIActive       = false;
            lsp_trace("UI has been deactivated");
            ui_deactivated();
        }

        void IModule::activate()
        {
            if (bActivated)
                return;

            bActivated      = true;
            activated();
            pWrapper->query_display_draw();
        }

        void IModule::deactivate()
        {
            if (!bActivated)
                return;

            bActivated      = false;
            deactivated();
            pWrapper->query_display_draw();
        }

        void IModule::init(IWrapper *wrapper)
        {
            pWrapper        = wrapper;
        }

        void IModule::set_sample_rate(long sr)
        {
            if (fSampleRate == sr)
                return;

            fSampleRate = sr;
            update_sample_rate(sr);
        };

        void IModule::update_sample_rate(long sr)
        {
        }

        void IModule::activated()
        {
        }

        void IModule::deactivated()
        {
        }

        void IModule::ui_activated()
        {
        }

        void IModule::ui_deactivated()
        {
        }

        void IModule::destroy()
        {
            vPorts.flush();
            bActivated      = false;
        }

        void IModule::update_settings()
        {
        }

        bool IModule::set_position(const position_t *pos)
        {
            return false;
        }

        void IModule::process(size_t samples)
        {
        }

        bool IModule::inline_display(ICanvas *cv, size_t width, size_t height)
        {
            return false;
        }

        util::KVTStorage *IModule::kvt_lock()
        {
            return (pWrapper != NULL) ? pWrapper->kvt_lock() : NULL;
        }

        util::KVTStorage *IModule::kvt_trylock()
        {
            return (pWrapper != NULL) ? pWrapper->kvt_trylock() : NULL;
        }

        void IModule::kvt_release()
        {
            if (pWrapper != NULL)
                pWrapper->kvt_release();
        }

        void IModule::state_saved()
        {
        }

        void IModule::state_loaded()
        {
        }

        void IModule::dump(dspu::IStateDumper *v) const
        {
        }
    }
}

