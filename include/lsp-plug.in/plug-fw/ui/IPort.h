/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IPORT_H_
#define LSP_PLUG_IN_PLUG_FW_UI_IPORT_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/lltl/ptrset.h>

namespace lsp
{
    namespace ui
    {
        class IPortListener;

        /**
         * Interface for UI port that can hold different types of data
         */
        class IPort
        {
            protected:
                const meta::port_t             *pMetadata;
                lltl::ptrset<IPortListener>     vListeners;

            public:
                explicit IPort(const meta::port_t *meta);
                virtual ~IPort();

            public:
                /** Add listener to the port
                 *
                 * @param listener that listens port changes
                 */
                void                bind(IPortListener *listener);

                /** Unbind listener
                 *
                 * @param listener listener to unbind
                 */
                void                unbind(IPortListener *listener);

                /** Unbind all controls
                 *
                 */
                void                unbind_all();

                /** Write some data to port
                 *
                 * @param buffer data to write to port
                 * @param size size of data
                 */
                virtual void        write(const void *buffer, size_t size);

                /** Write some data to port
                 *
                 * @param buffer data to write to port
                 * @param size size of data
                 * @param flags additional control flags
                 */
                virtual void        write(const void *buffer, size_t size, size_t flags);

                /** Get data from port
                 *
                 * @return associated buffer (may be NULL)
                 */
                virtual void       *buffer();

                /** Get single float value
                 *
                 * @return single float value
                 */
                virtual float       value();

                /** Get single default float value
                 *
                 * @return default float value
                 */
                virtual float       default_value();

                /**
                 * Set the value to the default
                 */
                virtual void        set_default();

                /** Set single float value
                 *
                 * @param value value to set
                 */
                virtual void        set_value(float value);

                /** Set single float value
                 *
                 * @param flags additional control flags @see port_flags_t
                 */
                virtual void        set_value(float value, size_t flags);

                /** Notify all that port data has been changed
                 * @param flags port notification flags, @see notify_flags_t
                 */
                virtual void        notify_all(size_t flags);

                /** Notify all that port metadata has been changed
                 *
                 */
                virtual void        sync_metadata();

             public:
                /** Get port metadata
                 *
                 * @return port metadata
                 */
                inline const meta::port_t      *metadata() const { return pMetadata; };

                /**
                 * Get unique port identifier
                 * @return unique port identifier
                 */
                virtual const char             *id() const;

                /** Get buffer casted to specified type
                 *
                 * @return buffer casted to specified type
                 */
                template <class T>
                    inline T *buffer()
                    {
                        return static_cast<T *>(buffer());
                    }
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UI_IPORT_H_ */
