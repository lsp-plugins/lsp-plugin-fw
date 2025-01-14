/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/plug-fw/core/JsonDumper.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace core
    {
        JsonDumper::JsonDumper()
        {
        }

        JsonDumper::~JsonDumper()
        {
            close();
        }

        void JsonDumper::init_params(json::serial_flags_t *flags)
        {
            flags->version      = json::JSON_LEGACY;
            flags->identifiers  = false;
            flags->ident        = ' ';
            flags->padding      = 4;
            flags->separator    = true;
            flags->multiline    = true;
        }

        status_t JsonDumper::open(const char *path)
        {
            json::serial_flags_t flags;
            init_params(&flags);

            return sOut.open(path, &flags);
        }

        status_t JsonDumper::open(const LSPString *path)
        {
            json::serial_flags_t flags;
            init_params(&flags);

            return sOut.open(path, &flags);
        }

        status_t JsonDumper::open(const io::Path *path)
        {
            json::serial_flags_t flags;
            init_params(&flags);

            return sOut.open(path, &flags);
        }

        status_t JsonDumper::close()
        {
            return sOut.close();
        }

        void JsonDumper::begin_raw_object(const char *name)
        {
            sOut.write_property(name);
            sOut.start_object();
        }

        void JsonDumper::begin_raw_object()
        {
            sOut.start_object();
        }

        void JsonDumper::end_raw_object()
        {
            sOut.end_object();
        }

        void JsonDumper::begin_object(const char *name, const void *ptr, size_t szof)
        {
            sOut.write_property(name);
            sOut.start_object();
            write("this", ptr);
            write("sizeof", szof);
            sOut.write_property("data");
            sOut.start_object();
        }

        void JsonDumper::begin_object(const void *ptr, size_t szof)
        {
            sOut.start_object();
            write("this", ptr);
            write("sizeof", szof);
            sOut.write_property("data");
            sOut.start_object();
        }

        void JsonDumper::end_object()
        {
            sOut.end_object();
            sOut.end_object();
        }

        void JsonDumper::begin_array(const char *name, const void *ptr, size_t count)
        {
            sOut.write_property(name);
            sOut.start_object();
            write("this", ptr);
            write("length", count);
            sOut.write_property("data");
            sOut.start_array();
        }

        void JsonDumper::begin_array(const void *ptr, size_t count)
        {
            sOut.start_object();
            write("this", ptr);
            write("length", count);
            sOut.write_property("data");
            sOut.start_array();
        }

        void JsonDumper::end_array()
        {
            sOut.end_array();
            sOut.end_object();
        }

        void JsonDumper::write(const void *value)
        {
            if (value == NULL)
            {
                sOut.write_null();
                return;
            }

            char buf[0x40];
            snprintf(buf, sizeof(buf), "*%p", value);
            sOut.write_string(buf);
        }

        void JsonDumper::write(const char *value)
        {
            if (value == NULL)
                sOut.write_null();
            else
                sOut.write_string(value);
        }

        void JsonDumper::write(bool value)
        {
            sOut.write_bool(value);
        }

        void JsonDumper::write(unsigned char value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(signed char value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(unsigned short value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(signed short value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(unsigned int value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(signed int value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(unsigned long value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(signed long value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(unsigned long long value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(signed long long value)
        {
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(float value)
        {
            sOut.write_double(value, "%g");
        }

        void JsonDumper::write(double value)
        {
            sOut.write_double(value, "%g");
        }

        void JsonDumper::write(const char *name, const void *value)
        {
            sOut.write_property(name);
            write(value);
        }

        void JsonDumper::write(const char *name, const char *value)
        {
            sOut.write_property(name);
            write(value);
        }

        void JsonDumper::write(const char *name, bool value)
        {
            sOut.write_property(name);
            sOut.write_bool(value);
        }

        void JsonDumper::write(const char *name, unsigned char value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, signed char value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, unsigned short value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, signed short value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, unsigned int value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, signed int value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, unsigned long value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, signed long value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, unsigned long long value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, signed long long value)
        {
            sOut.write_property(name);
            sOut.write_int(fixed_int(value));
        }

        void JsonDumper::write(const char *name, float value)
        {
            sOut.write_property(name);
            write(value);
        }

        void JsonDumper::write(const char *name, double value)
        {
            sOut.write_property(name);
            write(value);
        }

        void JsonDumper::writev(const void * const *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                write(value[i]);
            end_array();
        }

        void JsonDumper::writev(const bool *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                write(value[i]);
            end_array();
        }

        void JsonDumper::writev(const unsigned char *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const signed char *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const unsigned short *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const signed short *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const unsigned int *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const signed int *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const unsigned long *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const signed long *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const unsigned long long *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const signed long long *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const float *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                write(value[i]);
            end_array();
        }

        void JsonDumper::writev(const double *value, size_t count)
        {
            if (value == NULL)
            {
                write(value);
                return;
            }

            begin_array(value, count);
            for (size_t i=0; i<count; ++i)
                write(value[i]);
            end_array();
        }

        void JsonDumper::writev(const char *name, const void * const *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                write(value[i]);
            end_array();
        }

        void JsonDumper::writev(const char *name, const bool *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                write(value[i]);
            end_array();
        }

        void JsonDumper::writev(const char *name, const unsigned char *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const signed char *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const unsigned short *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const signed short *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const unsigned int *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const signed int *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const unsigned long *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const signed long *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const unsigned long long *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const signed long long *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                sOut.write_int(fixed_int(value[i]));
            end_array();
        }

        void JsonDumper::writev(const char *name, const float *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                write(value[i]);
            end_array();
        }

        void JsonDumper::writev(const char *name, const double *value, size_t count)
        {
            if (value == NULL)
            {
                write(name, value);
                return;
            }
            begin_array(name, value, count);
            for (size_t i=0; i<count; ++i)
                write(value[i]);
            end_array();
        }

    } /* namespace core */
} /* namespace lsp */

