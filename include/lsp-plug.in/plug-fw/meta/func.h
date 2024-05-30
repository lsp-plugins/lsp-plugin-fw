/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_META_FUNC_H_
#define LSP_PLUG_IN_PLUG_FW_META_FUNC_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/common/status.h>

namespace lsp
{
    namespace meta
    {
        enum estimation_t
        {
            EST_MIN,        // Minimum value but not inf, nan, etc
            EST_MAX,        // Maximum value
            EST_DFL,        // Default value
            EST_SPECIAL,    // Minimum value like inf, nan, etc
        };

        static inline bool is_out_port(const port_t *p)
        {
            switch (p->role)
            {
                case R_AUDIO_OUT:
                case R_METER:
                case R_MESH:
                case R_FBUFFER:
                case R_MIDI_OUT:
                case R_OSC_OUT:
                case R_STREAM:
                    return true;
                default:
                    break;
            }
            return false;
        }

        static inline bool is_in_port(const port_t *p)
        {
            switch (p->role)
            {
                case R_AUDIO_IN:
                case R_CONTROL:
                case R_PATH:
                case R_MIDI_IN:
                case R_PORT_SET:
                case R_OSC_IN:
                case R_BYPASS:
                    return true;
                default:
                    break;
            }
            return false;
        }

        static inline bool is_trigger_port(const port_t *p)
        {
            return p->flags & F_TRG;
        }

        static inline bool is_growing_port(const port_t *p)
        {
            return (p->flags & (F_GROWING | F_UPPER | F_LOWER)) == (F_GROWING | F_UPPER | F_LOWER);
        }

        static inline bool is_lowering_port(const port_t *p)
        {
            return (p->flags & (F_LOWERING | F_UPPER | F_LOWER)) == (F_LOWERING | F_UPPER | F_LOWER);
        }

        static inline bool is_audio_port(const port_t *p)
        {
            if (p == NULL)
                return false;
            return (p->role == R_AUDIO_IN) || (p->role == R_AUDIO_OUT);
        }

        static inline bool is_audio_in_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_AUDIO_IN);
        }

        static inline bool is_audio_out_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_AUDIO_OUT);
        }

        static inline bool is_midi_port(const port_t *p)
        {
            if (p == NULL)
                return false;
            return (p->role == R_MIDI_IN) || (p->role == R_MIDI_OUT);
        }

        static inline bool is_midi_in_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_MIDI_IN);
        }

        static inline bool is_midi_out_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_MIDI_OUT);
        }

        static inline bool is_osc_port(const port_t *p)
        {
            if (p == NULL)
                return false;
            return (p->role == R_OSC_IN) || (p->role == R_OSC_OUT);
        }

        static inline bool is_osc_in_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_OSC_IN);
        }

        static inline bool is_osc_out_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_OSC_OUT);
        }

        static inline bool is_control_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_CONTROL);
        }

        static inline bool is_bypass_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_BYPASS);
        }

        static inline bool is_port_set_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_PORT_SET);
        }

        static inline bool is_meter_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_METER);
        }

        static inline bool is_path_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_PATH);
        }

        static inline bool is_mesh_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_MESH);
        }

        static inline bool is_framebuffer_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_FBUFFER);
        }

        static inline bool is_stream_port(const port_t *p)
        {
            return (p != NULL) && (p->role == R_STREAM);
        }

        static inline bool is_optional_port(const port_t *p)
        {
            return (p != NULL) && (p->flags & F_OPTIONAL);
        }

        static inline size_t port_count(const meta::port_group_t *g)
        {
            size_t count = 0;
            for (const meta::port_group_item_t *i = g->items; (i != NULL) && (i->id != NULL); ++i)
                ++count;
            return count;
        }

        /**
         * Get name of the unit
         * @param unit unit_t unit
         * @return unit name (UTF-8 string), may be NULL
         */
        const char     *get_unit_name(size_t unit);

        /**
         * Get localized key for unit
         * @param unit unit_t unit
         * @return unit localized key, may be NULL
         */
        const char     *get_unit_lc_key(size_t unit);

        /**
         * Get unit by name
         * @param name unit name
         * @return unit, U_NONE if could not parse
         */
        unit_t          get_unit(const char *name);

        /**
         * Check that unit is of descrete type
         * @param unit unit_t unit
         * @return true if unit is of descrete type
         */
        bool            is_discrete_unit(size_t unit);

        /**
         * Check that unit is of descrete type
         * @param unit unit_t unit
         * @return true if unit is of descrete type
         */
        bool            is_bool_unit(size_t unit);

        /**
         * Check that unit is decibels
         * @param unit unit_t unit
         * @return true if unit is decibels
         */
        bool            is_decibel_unit(size_t unit);

        /**
         * Check that unit is gain
         * @param unit unit_t unit
         * @return true if unit is gain
         */
        bool            is_gain_unit(size_t unit);

        /**
         * Check that unit is degree unit
         * @param unit unit_t unit
         * @return true if unit is degree unit
         */
        bool            is_degree_unit(size_t unit);

        /**
         * Check that unit is enumerated unit
         * @param unit unit_t unit
         * @return true if unit is enumerated unit
         */
        bool            is_enum_unit(size_t unit);

        /**
         * Check that unit uses logarithmic rule
         * @param unit unit_t unit
         * @return true if unit uses logarithmic rule
         */
        bool            is_log_rule(const port_t *port);

        /**
         * Estimate the size of list (in elements) for the port type
         * @param list list to estimate size
         * @return size of list (in elements)
         */
        size_t          list_size(const port_item_t *list);

        /**
         * Limit floating-point value corresponding to the port's metadata
         * @param port port's metadata
         * @param value value to limit
         * @return limited value
         */
        float           limit_value(const port_t *port, float value);

        /**
         * Get all port parameters
         * @param p port metadata
         * @param min minimum value
         * @param max maximum value
         * @param step step
         */
        void            get_port_parameters(const port_t *p, float *min, float *max, float *step);

        /** Clone single port metadata
         *
         * @param metadata port list
         * @return cloned port metadata, should be freed by drop_port_metadata() call
         */
        port_t         *clone_single_port_metadata(const port_t *metadata);

        /** Clone multiple port metadata
         *
         * @param metadata port list
         * @param postfix potfix to be added to the port list, can be NULL
         * @return cloned port metadata, should be freed by drop_port_metadata() call
         */
        port_t         *clone_port_metadata(const port_t *metadata, const char *postfix);

        /** Drop port metadata
         *
         * @param metadata port metadata to drop
         */
        void            drop_port_metadata(port_t *metadata);

        /** Size of port list
         *
         * @param metadata port list metadata
         * @return number of elements excluding PORTS_END
         */
        size_t          port_list_size(const port_t *metadata);

        /**
         * Format floating-point value
         *
         * @param buf buffer to store formatted data
         * @param len length of the buffer
         * @param meta port metadata
         * @param value value to format
         * @param precision precision
         * @param units emit units to the final format if possible
         */
        void            format_float(char *buf, size_t len, const port_t *meta, float value, ssize_t precision, bool units);

        /**
         * Format integer value
         *
         * @param buf buffer to store formatted data
         * @param len length of the buffer
         * @param meta port metadata
         * @param value value to format
         * @param units emit units to the final format if possible
         */
        void            format_int(char *buf, size_t len, const port_t *meta, float value, bool units);

        /**
         * Format enumerated value
         *
         * @param buf buffer to store formatted data
         * @param len length of the buffer
         * @param meta port metadata
         * @param value value to format
         */
        void            format_enum(char *buf, size_t len, const port_t *meta, float value);

        /**
         * Format floating-point value in decibel units
         *
         * @param buf buffer to store formatted data
         * @param len length of the buffer
         * @param meta port metadata
         * @param value value to format
         * @param precision precision
         * @param units emit units to the final format if possible
         */
        void            format_decibels(char *buf, size_t len, const port_t *meta, float value, ssize_t precision, bool units);

        /**
         * Format boolean value
         * @param buf buffer to store formatted data
         * @param len length of the buffer
         * @param meta port metadata
         * @param value value to format
         */
        void            format_bool(char *buf, size_t len, const port_t *meta, float value);

        /**
         * Format the value
         * @param buf buffer to store formatted data
         * @param len length of the buffer
         * @param meta port metadata
         * @param value value to format
         * @param precision precision
         * @param units emit units to the final format if possible
         */
        void            format_value(char *buf, size_t len, const port_t *meta, float value, ssize_t precision, bool units);

        /**
         * Form the esimation string for a value
         * @param buf buffer to store estimation pattern
         * @param len length of the buffer
         * @param meta port metadata
         * @param units use units
         * @return true if estimation is supported
         */
        bool            estimate_value(char *buf, size_t len, const port_t *meta, estimation_t e, ssize_t precision, bool units);

        /**
         * Parse some text value associated with specified metadata and considered to be boolean
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @return status of operation
         */
        status_t        parse_bool(float *dst, const char *text, const port_t *meta);

        /**
         * Parse some text value associated with specified metadata and considered to be enumeration
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @return status of operation
         */
        status_t        parse_enum(float *dst, const char *text, const port_t *meta);

        /**
         * Parse some text value associated with specified metadata and considered to be decibel value
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @param units allow units passed after a value
         * @return status of operation
         */
        status_t        parse_decibels(float *dst, const char *text, const port_t *meta, bool units);

        /**
         * Try to convert the string to the note name and compute it's main tone frequency considering
         * the A2 being 440 Hz.
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @return status of operation
         */
        status_t        parse_note_frequency(float *dst, const char *text, const port_t *meta);

        /**
         * Parse frequency value. Allows to pass note name considering the A2 being 440 Hz.
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @param units allow units to be specified if possible
         * @return status of operation
         */
        status_t        parse_frequency(float *dst, const char *text, const port_t *meta, bool units);

        /**
         * Parse time value.
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @param units allow units to be specified if possible
         * @return status of operation
         */
        status_t        parse_time(float *dst, const char *text, const port_t *meta, bool units);

        /**
         * Parse some text value associated with specified metadata and considered to be integer value
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @param units allow units to be specified if possible
         * @return status of operation
         */
        status_t        parse_int(float *dst, const char *text, const port_t *meta, bool units);

        /**
         * Parse some text value associated with specified metadata and considered to be floating-point value
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @param units allow units to be specified if possible
         * @return status of operation
         */
        status_t        parse_float(float *dst, const char *text, const port_t *meta, bool units);

        /**
         * Parse some text value associated with specified metadata
         *
         * @param dst destination pointer to store parsed value
         * @param text text to parse
         * @param meta associated metadata
         * @param units allow units passed after a value
         * @return status of operation
         */
        status_t        parse_value(float *dst, const char *text, const port_t *meta, bool units);

        /**
         * Check that value matches the range specified by the port metadata
         * @param meta port metadata
         * @param value value to match
         * @return true if the value matches the range
         */
        bool            range_match(const port_t *meta, float value);

        /**
         * Convert VST2 identifier to VST3 identifier
         * @param buf pointer to buffer to store value, should be at least 33 characters length
         * @param vst2_uid original string representation of VST2 identifier
         * @param name plugin name
         * @param for_controller make VST3 identifier for controller
         * @return pointer to the string stored in the buffer or NULL on error
         */
        char           *uid_vst2_to_vst3(char *buf, const char *vst2_uid, const char *name, bool for_controller = false);

        /**
         * Convert VST3 unique identifier to TUID
         * @param tuid VST3 TUID
         * @param vst3_uid VST3 unique identifier (32-character hex string or 16-character ASCII string)
         * @return true if conversion succeeded
         */
        bool            uid_vst3_to_tuid(char *tuid, const char *vst3_uid);

        /**
         * Convert VST3 unique identifier to TUID
         * @param vst3_uid VST3 unique identifier (32-character hex string, uppercase), should be of at least 33 bytes length
         * @param tuid VST3 TUID of 16 characters
         * @return pointer to converted string or NULL on error
         */
        char           *uid_tuid_to_vst3(char *vst3_uid, const char *tuid);

        /**
         * Convert VST3 identifier as it is stored in metadata to VST3 identifier as it is used to be presented by
         * the VST3 standard.
         *
         * @param vst3_uid pointer to store identifier (32-character hex string, uppercase), should be of at least 33 bytes length
         * @param meta_uid original identifier stored in metadata (16 NULL-terminated ASCII characters)
         * @return pointer to value stored in vst3_uid or NULL on error
         */
        char           *uid_meta_to_vst3(char *vst3_uid, const char *meta_uid);

        /**
         * Return plugin format name by the format specifier
         * @param format plugin format specifier
         * @return plugin format name
         */
        const char     *plugin_format_name(plugin_format_t format);
    } /* namespace meta */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_META_FUNC_H_ */
