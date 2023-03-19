/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 янв. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_HELPERS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_HELPERS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace clap
    {
        /**
         * Perform the string copy with guaranteed string termination at the end
         * @param dst destination buffer
         * @param src source buffer
         * @param len length of the buffer
         * @return pointer to destination buffer
         */
        inline char *clap_strcpy(char *dst, const char *src, size_t len)
        {
            strncpy(dst, src, len-1);
            dst[len-1] = '\0';
            return dst;
        }

        /**
         * Hash the string value and return the hash value as a clap identifier
         * @param str string to hash
         * @return clap identifier as a result of hashing
         */
        inline clap_id clap_hash_string(const char *str)
        {
            constexpr size_t num_primes = 8;
            static const uint16_t primes[num_primes] = {
                0x80ab, 0x815f, 0x8d41, 0x9161,
                0x9463, 0x9b77, 0xabc1, 0xb567,
            };

            size_t prime_id = 0;
            size_t len      = strlen(str);
            clap_id res     = len * primes[prime_id];

            for (size_t i=0; i<len; ++i)
            {
                prime_id        = (prime_id + 1) % num_primes;
                res             = clap_id(res << 7) | clap_id((res >> (sizeof(clap_id) * 8 - 7)) & 0x7f); // rotate 7 bits left
                res            += str[i] * primes[prime_id];
            }

            return res;
        }

        inline plug::mesh_t *create_mesh(const meta::port_t *meta)
        {
            size_t buffers      = meta->step;
            size_t buf_size     = meta->start * sizeof(float);
            size_t mesh_size    = sizeof(plug::mesh_t) + sizeof(float *) * buffers;

            // Align values to 64-byte boundaries
            buf_size            = align_size(buf_size, 0x40);
            mesh_size           = align_size(mesh_size, 0x40);

            // Allocate pointer
            uint8_t *ptr        = static_cast<uint8_t *>(malloc(mesh_size + buf_size * buffers));
            if (ptr == NULL)
                return NULL;

            // Initialize references
            plug::mesh_t *mesh  = reinterpret_cast<plug::mesh_t *>(ptr);
            mesh->nState        = plug::M_EMPTY;
            mesh->nBuffers      = 0;
            mesh->nItems        = 0;
            ptr                += mesh_size;
            for (size_t i=0; i<buffers; ++i)
            {
                mesh->pvData[i]     = reinterpret_cast<float *>(ptr);
                ptr                += buf_size;
            }

            return mesh;
        }

        inline void destroy_mesh(plug::mesh_t *mesh)
        {
            if (mesh != NULL)
                free(mesh);
        }

        /**
         * Write the constant-sized block to the CLAP output streem
         * @param os CLAP output stream
         * @param buf buffer that should be written
         * @param size size of buffer to write
         * @return status of operation
         */
        inline status_t write_fully(const clap_ostream_t *os, const void *buf, size_t size)
        {
            const uint8_t *ptr = static_cast<const uint8_t *>(buf);
            for (size_t offset = 0; offset < size; )
            {
                ssize_t written = os->write(os, &ptr[offset], size - offset);
                if (written < 0)
                    return STATUS_IO_ERROR;
                offset         += written;
            }
            return STATUS_OK;
        }

        /**
         * Write simple data type to the CLAP output stream
         * @param os CLAP output stream
         * @param value value to write
         * @return status of operation
         */
        template <class T>
        inline status_t write_fully(const clap_ostream_t *os, const T &value)
        {
            T tmp   = CPU_TO_LE(value);
            return write_fully(os, &tmp, sizeof(tmp));
        }

        /**
         * Read the constant-sized block from the CLAP input streem
         * @param is CLAP input stream
         * @param buf target buffer to read the data to
         * @param size size of buffer to read
         * @return status of operation
         */
        inline status_t read_fully(const clap_istream_t *is, void *buf, size_t size)
        {
            uint8_t *ptr = static_cast<uint8_t *>(buf);
            for (size_t offset = 0; offset < size; )
            {
                ssize_t read = is->read(is, &ptr[offset], size - offset);
                if (read <= 0)
                {
                    if (read < 0)
                        return STATUS_IO_ERROR;
                    return (offset > 0) ? STATUS_CORRUPTED : STATUS_EOF;
                }
                offset         += read;
            }
            return STATUS_OK;
        }

        /**
         * Read simple data type from the CLAP input stream
         * @param is CLAP input stream
         * @param value value to write
         * @return status of operation
         */
        template <class T>
        inline status_t read_fully(const clap_istream_t *is, T *value)
        {
            T tmp;
            status_t res = read_fully(is, &tmp, sizeof(tmp));
            if (res == STATUS_OK)
                *value      = LE_TO_CPU(tmp);
            return STATUS_OK;
        }

        inline status_t write_varint(const clap_ostream_t *os, size_t value)
        {
            do {
                uint8_t b   = (value >= 0x80) ? 0x80 | (value & 0x7f) : value;
                value     >>= 7;

                ssize_t n   = os->write(os, &b, sizeof(b));
                if (n < 0)
                    return STATUS_IO_ERROR;
            } while (value > 0);

            return STATUS_OK;
        }

        /**
         * Write string to CLAP output stream
         * @param os CLAP output stream
         * @param s NULL_terminated string to write
         * @return number of actual bytes written or negative error code
         */
        inline status_t write_string(const clap_ostream_t *os, const char *s)
        {
            size_t len = strlen(s);

            // Write variable-sized string length
            status_t res = write_varint(os, len);
            if (res != STATUS_OK)
                return res;

            // Write the payload data
            return write_fully(os, s, len);
        }

        /**
         * Read the variable-sized integer
         * @param is input stream to perform read
         * @param value the pointer to store the read value
         * @return status of operation
         */
        inline status_t read_varint(const clap_istream_t *is, size_t *value)
        {
            // Read variable-sized string length
            size_t len = 0, shift = 0;
            while (true)
            {
                uint8_t b;
                ssize_t n = is->read(is, &b, sizeof(b));
                if (n <= 0)
                {
                    if (n < 0)
                        return STATUS_IO_ERROR;
                    return (shift > 0) ? STATUS_CORRUPTED : STATUS_EOF;
                }

                // Commit part of the value to the result variable
                len    |= size_t(b & 0x7f) << shift;
                if (!(b & 0x80)) // Last byte in the sequence?
                    break;
                shift += 7;
                if (shift > ((sizeof(size_t) * 8) - 7))
                    return STATUS_OVERFLOW;
            }

            *value = len;
            return STATUS_OK;
        }

        /**
         * Read the string from the CLAP input stream
         * @param is CLAP input stream
         * @param buf buffer to store the string
         * @param maxlen the maximum available string length. @note The value should consider
         *        that the destination buffer holds at least one more character for NULL-terminating
         *        character
         * @return number of actual bytes read or negative error code
         */
        inline status_t read_string(const clap_istream_t *is, char *buf, size_t maxlen)
        {
            // Read variable-sized string length
            size_t len = 0;
            status_t res = read_varint(is, &len);
            if (res != STATUS_OK)
                return res;
            if (len > maxlen)
                return STATUS_OVERFLOW;

            // Read the payload data
            res = read_fully(is, buf, len);
            if (res == STATUS_OK)
                buf[len]  = '\0';
            return STATUS_OK;
        }

        /**
         * Read the string from the CLAP input stream
         * @param is CLAP input stream
         * @param buf pointer to variable to store the pointer to the string. The previous value will be
         *   reallocated if there is not enough capacity. Should be freed by caller after use even if the
         *   execution was unsuccessful.
         * @param capacity the pointer to variable that contains the current capacity of the string
         * @return number of actual bytes read or negative error code
         */
        inline status_t read_string(const clap_istream_t *is, char **buf, size_t *capacity)
        {
            // Read variable-sized string length
            size_t len = 0;
            status_t res = read_varint(is, &len);
            if (res != STATUS_OK)
                return res;

            // Reallocate memory if there is not enough space
            char *s     = *buf;
            size_t cap  = *capacity;
            if ((s == NULL) || (cap < (len + 1)))
            {
                cap     = align_size(len + 1, 32);
                s       = static_cast<char *>(realloc(s, sizeof(char *) * cap));
                if (s == NULL)
                    return STATUS_NO_MEM;

                *buf        = s;
                *capacity   = cap;
            }

            // Read the payload data
            res = read_fully(is, s, len);
            if (res == STATUS_OK)
                s[len]      = '\0';

            return STATUS_OK;
        }

        inline float to_clap_value(const meta::port_t *meta, float value, float *min_value, float *max_value)
        {
            float min = 0.0f, max = 1.0f, step = 0.0f;
            meta::get_port_parameters(meta, &min, &max, &step);

//                lsp_trace("input = %.3f", value);
            // Set value as integer or normalized
            if (meta::is_gain_unit(meta->unit))
            {
//                float p_value   = value;

                float base      = (meta->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;
                float thresh    = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                float l_step    = log(step + 1.0f) * 0.1f;
                float l_thresh  = log(thresh);
                float l_value   = (fabs(value) < thresh) ? (l_thresh - l_step) : (log(value));

                value           = l_value * base;
//                lsp_trace("%s = %f (%f, %f, %f) -> %f (%f)",
//                    meta->id,
//                    p_value,
//                    min, max, step,
//                    value,
//                    l_thresh);

                min             = (fabs(min)   < thresh) ? (l_thresh - l_step) * base : (log(min) * base);
                max             = (fabs(max)   < thresh) ? (l_thresh - l_step) * base : (log(max) * base);
            }
            else if (meta::is_log_rule(meta))
            {
//                        float p_value   = value;

                float thresh    = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                float l_step    = log(step + 1.0f) * 0.1f;
                float l_thresh  = log(thresh);

                float l_min     = (fabs(min)   < thresh) ? (l_thresh - l_step) : (log(min));
                float l_max     = (fabs(max)   < thresh) ? (l_thresh - l_step) : (log(max));
                float l_value   = (fabs(value) < thresh) ? (l_thresh - l_step) : (log(value));

                value           = (l_value - l_min) / (l_max - l_min);

                min             = 0.0f;
                max             = 1.0f;
            }
            else if (meta->unit == meta::U_BOOL)
            {
                value = (value >= (min + max) * 0.5f) ? 1.0f : 0.0f;
            }
            else
            {
                if ((meta->flags & meta::F_INT) ||
                    (meta->unit == meta::U_ENUM) ||
                    (meta->unit == meta::U_SAMPLES))
                    value  = truncf(value);

                // Normalize value
                value = (max != min) ? (value - min) / (max - min) : 0.0f;
            }

            if (min_value != NULL)
                *min_value      = min;
            if (max_value != NULL)
                *max_value      = max;

//                lsp_trace("result = %.3f", value);
            return value;
        }

        inline float from_clap_value(const meta::port_t *meta, float value)
        {
//                lsp_trace("input = %.3f", value);
            // Set value as integer or normalized
            float min = 0.0f, max = 1.0f, step = 0.0f;
            meta::get_port_parameters(meta, &min, &max, &step);

            if (meta::is_gain_unit(meta->unit))
            {
//                float p_value   = value;

                float base      = (meta->unit == meta::U_GAIN_AMP) ? M_LN10 / 20.0 : M_LN10 / 10.0;
                float thresh    = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                float l_thresh  = log(thresh);

                value           = value * base;
                value           = (value < l_thresh) ? 0.0f : expf(value);

//                lsp_trace("%s = %f (%f) -> %f (%f, %f, %f)",
//                    meta->id,
//                    p_value,
//                    l_thresh,
//                    value,
//                    min, max, step);
            }
            else if (meta::is_log_rule(meta))
            {
//                        float p_value   = value;
                float thresh    = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                float l_step    = log(step + 1.0f) * 0.1f;
                float l_thresh  = log(thresh);
                float l_min     = (fabs(min)   < thresh) ? (l_thresh - l_step) : (log(min));
                float l_max     = (fabs(max)   < thresh) ? (l_thresh - l_step) : (log(max));

                value           = value * (l_max - l_min) + l_min;
                value           = (value < l_thresh) ? 0.0f : expf(value);

//                        lsp_trace("%s = %f (%f, %f, %f) -> %f (%f, %f, %f)",
//                            pMetadata->id,
//                            p_value,
//                            l_thresh, l_min, l_max,
//                            value,
//                            min, max, step);
            }
            else if (meta->unit == meta::U_BOOL)
            {
                value = (value >= 0.5f) ? max : min;
            }
            else
            {
                value = min + value * (max - min);
                if ((meta->flags & meta::F_INT) ||
                    (meta->unit == meta::U_ENUM) ||
                    (meta->unit == meta::U_SAMPLES))
                    value  = truncf(value);
            }

//                lsp_trace("result = %.3f", value);
            return value;
        }

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_HELPERS_H_ */
