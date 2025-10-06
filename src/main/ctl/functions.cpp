/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 сент. 2025 г.
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

#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/expr/Variables.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace ctl
    {
        static inline bool midi_note_valid(ssize_t value)
        {
            return (value >= 0) && (value <= 127);
        }

        static inline status_t get_midi_note(ssize_t *dst, const expr::value_t *value)
        {
            ssize_t midi_note = 0;

            status_t res = (value->type != expr::VT_STRING) ?
                expr::fetch_as_int(&midi_note, value) :
                meta::parse_note_number(&midi_note, value->v_str->get_utf8());

            if (res != STATUS_OK)
                return res;

            if (!midi_note_valid(midi_note))
                return STATUS_INVALID_VALUE;

            *dst = midi_note;
            return STATUS_OK;
        }

        static inline status_t parse_midi_note(ssize_t *note, float *base, size_t num_args, const expr::value_t *args)
        {
            // Check arguments
            if ((num_args < 1) || (num_args > 2))
                return STATUS_BAD_ARGUMENTS;

            // Create temporary variables
            status_t res;
            float r_base = 440.0f;
            ssize_t r_note = -1;

            // Read MIDI note code
            if ((res = get_midi_note(&r_note, &args[0])) != STATUS_OK)
                return res;

            // Read base frequency
            if (num_args > 1)
            {
                if ((res = expr::fetch_as_float(&r_base, &args[1])) != STATUS_OK)
                    return res;
            }

            // Return result
            *note       = r_note;
            *base       = r_base;

            return STATUS_OK;
        }

        static inline status_t parse_midi_range(ssize_t *start, ssize_t *end, float *base, size_t num_args, const expr::value_t *args)
        {
            // Check arguments
            if ((num_args < 2) || (num_args > 3))
                return STATUS_BAD_ARGUMENTS;

            // Create temporary variables
            status_t res;
            float r_base = 440.0f;
            ssize_t r_start = -1, r_end = -1;

            // Read start MIDI note code
            if ((res = get_midi_note(&r_start, &args[0])) != STATUS_OK)
                return res;

            // Read end MIDI note code
            if ((res = parse_midi_note(&r_end, &r_base, num_args - 1, &args[1])) != STATUS_OK)
                return res;

            // Return result
            *start      = r_start;
            *end        = r_end;
            *base       = r_base;

            return STATUS_OK;
        }

        static status_t midi_freq(void *context, expr::value_t *result, size_t num_args, const expr::value_t *args)
        {
            // Read start MIDI note code
            status_t res;
            float base = 440.0f;
            ssize_t note = -1;

            if ((res = parse_midi_note(&note, &base, num_args, args)) != STATUS_OK)
                return res;

            const float freq    = dspu::midi_note_to_frequency(note, base);
            expr::set_value_float(result, freq);

            return STATUS_OK;
        }

        static status_t midi_freq_start(void *context, expr::value_t *result, size_t num_args, const expr::value_t *args)
        {
            // Read start MIDI note code
            status_t res;
            float base = 440.0f;
            ssize_t start = -1, end = -1;

            if ((res = parse_midi_range(&start, &end, &base, num_args, args)) != STATUS_OK)
                return res;

            const float f_start = dspu::midi_note_to_frequency(start, base);
            const float f_end   = dspu::midi_note_to_frequency(end, base);
            const float count   = end - start;
            const float delta   = expf(logf(f_end/f_start) / (2.0f - 2.0f * count)); // delta = half note range

            expr::set_value_float(result, f_start * delta);

            return STATUS_OK;
        }

        static status_t midi_freq_end(void *context, expr::value_t *result, size_t num_args, const expr::value_t *args)
        {
            // Read start MIDI note code
            status_t res;
            float base = 440.0f;
            ssize_t start = -1, end = -1;

            if ((res = parse_midi_range(&start, &end, &base, num_args, args)) != STATUS_OK)
                return res;

            const float f_start = dspu::midi_note_to_frequency(start, base);
            const float f_end   = dspu::midi_note_to_frequency(end, base);
            const float count   = end - start;
            const float delta   = expf(logf(f_end/f_start) / (count - 1.0f));       // delta = full note range

            expr::set_value_float(result, f_end * delta);

            return STATUS_OK;
        }

        status_t bind_functions(expr::Variables *vars)
        {
            LSP_STATUS_ASSERT(vars->bind_func("midi_freq", midi_freq));
            LSP_STATUS_ASSERT(vars->bind_func("midi_freq_start", midi_freq_start));
            LSP_STATUS_ASSERT(vars->bind_func("midi_freq_end", midi_freq_end));

            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


