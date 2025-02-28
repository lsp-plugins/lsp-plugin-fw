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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_JSONDUMPER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_JSONDUMPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>
#include <lsp-plug.in/fmt/json/Serializer.h>

namespace lsp
{
    namespace core
    {
        /**
         * Dumper of plugin state into JSON output format
         */
        class JsonDumper: public dspu::IStateDumper
        {
            protected:
                json::Serializer sOut;

            protected:
                static void init_params(json::serial_flags_t *flags);

            public:
                explicit JsonDumper();
                JsonDumper(const JsonDumper &) = delete;
                JsonDumper(JsonDumper &&) = delete;
                virtual ~JsonDumper();

                JsonDumper & operator = (const JsonDumper &) = delete;
                JsonDumper & operator = (JsonDumper &&) = delete;

            public:
                status_t    open(const char *path);
                status_t    open(const io::Path *path);
                status_t    open(const LSPString *path);
                status_t    close();

            public:
                void begin_raw_object(const char *name);
                void begin_raw_object();
                void end_raw_object();

                virtual void begin_object(const char *name, const void *ptr, size_t szof);
                virtual void begin_object(const void *ptr, size_t szof);
                virtual void end_object();

                virtual void begin_array(const char *name, const void *ptr, size_t length);
                virtual void begin_array(const void *ptr, size_t length);
                virtual void end_array();

                virtual void write(const void *value);
                virtual void write(const char *value);
                virtual void write(bool value);
                virtual void write(unsigned char value);
                virtual void write(signed char value);
                virtual void write(unsigned short value);
                virtual void write(signed short value);
                virtual void write(unsigned int value);
                virtual void write(signed int value);
                virtual void write(unsigned long value);
                virtual void write(signed long value);
                virtual void write(unsigned long long value);
                virtual void write(signed long long value);
                virtual void write(float value);
                virtual void write(double value);

                virtual void write(const char *name, const void *value);
                virtual void write(const char *name, const char *value);
                virtual void write(const char *name, bool value);
                virtual void write(const char *name, unsigned char value);
                virtual void write(const char *name, signed char value);
                virtual void write(const char *name, unsigned short value);
                virtual void write(const char *name, signed short value);
                virtual void write(const char *name, unsigned int value);
                virtual void write(const char *name, signed int value);
                virtual void write(const char *name, unsigned long value);
                virtual void write(const char *name, signed long value);
                virtual void write(const char *name, unsigned long long value);
                virtual void write(const char *name, signed long long value);
                virtual void write(const char *name, float value);
                virtual void write(const char *name, double value);

                virtual void writev(const void * const *value, size_t count);
                virtual void writev(const bool *value, size_t count);
                virtual void writev(const unsigned char *value, size_t count);
                virtual void writev(const signed char *value, size_t count);
                virtual void writev(const unsigned short *value, size_t count);
                virtual void writev(const signed short *value, size_t count);
                virtual void writev(const unsigned int *value, size_t count);
                virtual void writev(const signed int *value, size_t count);
                virtual void writev(const unsigned long *value, size_t count);
                virtual void writev(const signed long *value, size_t count);
                virtual void writev(const unsigned long long *value, size_t count);
                virtual void writev(const signed long long *value, size_t count);
                virtual void writev(const float *value, size_t count);
                virtual void writev(const double *value, size_t count);

                virtual void writev(const char *name, const void * const *value, size_t count);
                virtual void writev(const char *name, const bool *value, size_t count);
                virtual void writev(const char *name, const unsigned char *value, size_t count);
                virtual void writev(const char *name, const signed char *value, size_t count);
                virtual void writev(const char *name, const unsigned short *value, size_t count);
                virtual void writev(const char *name, const signed short *value, size_t count);
                virtual void writev(const char *name, const unsigned int *value, size_t count);
                virtual void writev(const char *name, const signed int *value, size_t count);
                virtual void writev(const char *name, const unsigned long *value, size_t count);
                virtual void writev(const char *name, const signed long *value, size_t count);
                virtual void writev(const char *name, const unsigned long long *value, size_t count);
                virtual void writev(const char *name, const signed long long *value, size_t count);
                virtual void writev(const char *name, const float *value, size_t count);
                virtual void writev(const char *name, const double *value, size_t count);

        };

    } /* namespace core */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CORE_JSONDUMPER_H_ */
