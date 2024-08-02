/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 апр. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_MESSAGE_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_MESSAGE_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <steinberg/vst3.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/lltl/pphash.h>

namespace lsp
{
    namespace vst3
    {
        class Message:
            public Steinberg::Vst::IMessage,
            public Steinberg::Vst::IAttributeList
        {
            protected:
                enum data_type_t
                {
                    INT,
                    FLOAT,
                    STRING,
                    BLOB
                };

                typedef struct item_t
                {
                    uint32_t    type;
                    uint32_t    size;
                    uint8_t     data[];
                } item_t;

            protected:
                uatomic_t                   nRefCounter;    // Reference counter
                Steinberg::char8           *sMessageId;
                lltl::pphash<char, item_t>  vItems;

            protected:
                Steinberg::tresult set_item(AttrID id, data_type_t type, const void *value, size_t bytes);
                item_t *get_item(AttrID id, data_type_t type);

            public:
                Message();
                Message(const Message &) = delete;
                Message(Message &&) = delete;
                virtual ~Message();

                Message & operator = (const Message &) = delete;
                Message & operator = (Message &&) = delete;

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32 PLUGIN_API addRef() override;
                virtual Steinberg::uint32 PLUGIN_API release() override;

            public: // Steinberg::Vst::IMessage
                virtual Steinberg::FIDString PLUGIN_API getMessageID() override;
                virtual void PLUGIN_API setMessageID(Steinberg::FIDString id) override;
                virtual Steinberg::Vst::IAttributeList * PLUGIN_API getAttributes() override;

            public:
                virtual Steinberg::tresult PLUGIN_API setInt(AttrID id, Steinberg::int64 value) override;
                virtual Steinberg::tresult PLUGIN_API getInt(AttrID id, Steinberg::int64 & value) override;
                virtual Steinberg::tresult PLUGIN_API setFloat(AttrID id, double value) override;
                virtual Steinberg::tresult PLUGIN_API getFloat(AttrID id, double & value) override;
                virtual Steinberg::tresult PLUGIN_API setString(AttrID id, const Steinberg::Vst::TChar * string) override;
                virtual Steinberg::tresult PLUGIN_API getString(AttrID id, Steinberg::Vst::TChar* string, Steinberg::uint32 sizeInBytes) override;
                virtual Steinberg::tresult PLUGIN_API setBinary(AttrID id, const void *data, Steinberg::uint32 sizeInBytes) override;
                virtual Steinberg::tresult PLUGIN_API getBinary(AttrID id, const void *& data, Steinberg::uint32& sizeInBytes) override;
        };

    } /* namespace vst3 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_MESSAGE_H_ */
