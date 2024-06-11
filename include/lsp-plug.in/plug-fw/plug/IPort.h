/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plug-fw
 * Created on: 22 окт. 2015 г.
 *
 * lsp-plug-fw is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plug-fw is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plug-fw. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_PLUG_IPORT_H_
#define LSP_PLUG_IN_PLUG_FW_PLUG_IPORT_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

namespace lsp
{
    namespace plug
    {
        enum port_flags_t
        {
            PF_NONE             = 0,            // No flags
            PF_STATE_RESTORE    = 1 << 1,       // Port's state restore
            PF_STATE_IMPORT     = 1 << 2,       // Port's state import
            PF_PRESET_IMPORT    = 1 << 3        // Port's preset import
        };

        /**
         * Absract parameter port that may contain different types of data
         */
        class IPort
        {
            protected:
                const meta::port_t     *pMetadata;

            public:
                explicit IPort(const meta::port_t *meta);
                IPort(const IPort &) = delete;
                IPort(IPort &&) = delete;
                virtual ~IPort();

                IPort & operator = (const IPort &) = delete;
                IPort & operator = (IPort &&) = delete;

            public:
                /** Get port value
                 *
                 * @return port value or default metadata value if not connected/initialized
                 */
                virtual float value();

                /** Get default port value
                 *
                 * @return default port value
                 */
                virtual float default_value() const;

                /** Set port value
                 *
                 * @param value value to set
                 */
                virtual void set_value(float value);

                /** Set port value to default
                 */
                virtual void set_default();

                /** Get port buffer, may be NULL if buffer write is not required
                 *
                 */
                virtual void *buffer();

            public:
                /**
                 * Get port metadata
                 * @return port metadata
                 */
                inline const meta::port_t *metadata() const { return pMetadata; };

                /**
                 * Get port identifier
                 * @return port identifier
                 */
                inline const char *id() const               { return (pMetadata != NULL) ? pMetadata->id : NULL; }

                /** Get buffer casted to specified type
                 *
                 * @return buffer casted to specified type
                 */
                template <class T> inline T *buffer()
                {
                    return static_cast<T *>(buffer());
                }
        };
    } /* namespace plug */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_PLUG_IPORT_H_ */
