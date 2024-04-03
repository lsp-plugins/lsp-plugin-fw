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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_MESSAGE_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_MESSAGE_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/message.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/debug.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>

namespace lsp
{
    namespace vst3
    {

        Message::Message()
        {
            nRefCounter     = 1;
            sMessageId      = NULL;
//            lsp_trace("this = %p" , this);
        }

        Message::~Message()
        {
            if (sMessageId != NULL)
            {
                free(sMessageId);
                sMessageId      = NULL;
            }

            for (lltl::iterator<item_t> it = vItems.values(); it; ++it)
                free(it.get());
            vItems.flush();

//            lsp_trace("this = %p" , this);
        }

        Steinberg::tresult PLUGIN_API Message::queryInterface(const Steinberg::TUID _iid, void **obj)
        {
            IF_TRACE(
                char dump[36];
                lsp_trace("this=%p, _iid=%s",
                    this,
                    meta::uid_tuid_to_vst3(dump, _iid));
            );

            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                return cast_interface<Steinberg::FUnknown>(static_cast<Steinberg::Vst::IMessage *>(this), obj);
            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IMessage::iid))
                return cast_interface<Steinberg::Vst::IMessage>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IAttributeList::iid))
                return cast_interface<Steinberg::Vst::IAttributeList>(this, obj);

            return no_interface(obj);
        }

        Steinberg::uint32 PLUGIN_API Message::addRef()
        {
            return atomic_add(&nRefCounter, 1) + 1;
        }

        Steinberg::uint32 PLUGIN_API Message::release()
        {
            atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        Steinberg::tresult Message::set_item(AttrID id, data_type_t type, const void *value, size_t bytes)
        {
            if ((id == NULL) || (value == NULL))
                return Steinberg::kInvalidArgument;

            item_t *item = static_cast<item_t *>(malloc(sizeof(item_t) + bytes));
            if (item == NULL)
                return Steinberg::kOutOfMemory;
            lsp_finally {
                if (item != NULL)
                    free(item);
            };

            item->type = type;
            item->size = bytes;
            memcpy(item->data, value, bytes);

            if (!vItems.put(id, item, &item))
                return Steinberg::kOutOfMemory;

            return Steinberg::kResultOk;
        }

        Message::item_t *Message::get_item(AttrID id, data_type_t type)
        {
            if (id == NULL)
                return NULL;

            item_t *item = vItems.get(id);
            if (item == NULL)
                return NULL;

            return (item->type == type) ? item : NULL;
        }

        Steinberg::FIDString PLUGIN_API Message::getMessageID()
        {
            return sMessageId;
        }

        void PLUGIN_API Message::setMessageID(Steinberg::FIDString id)
        {
            Steinberg::char8 *msg_id = (id != NULL) ? strdup(id) : NULL;
            lsp::swap(msg_id, sMessageId);
            if (msg_id != NULL)
                free(msg_id);
        }

        Steinberg::Vst::IAttributeList * PLUGIN_API Message::getAttributes()
        {
            return this;
        }

        Steinberg::tresult PLUGIN_API Message::setInt(AttrID id, Steinberg::int64 value)
        {
            return set_item(id, INT, &value, sizeof(value));
        }

        Steinberg::tresult PLUGIN_API Message::getInt(AttrID id, Steinberg::int64 & value)
        {
            item_t *item = get_item(id, INT);
            if (item == NULL)
                return Steinberg::kInvalidArgument;

            memcpy(&value, item->data, sizeof(value));

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Message::setFloat(AttrID id, double value)
        {
            return set_item(id, FLOAT, &value, sizeof(value));
        }

        Steinberg::tresult PLUGIN_API Message::getFloat(AttrID id, double & value)
        {
            item_t *item = get_item(id, FLOAT);
            if (item == NULL)
                return Steinberg::kInvalidArgument;

            memcpy(&value, item->data, sizeof(value));

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Message::setString(AttrID id, const Steinberg::Vst::TChar * string)
        {
            if (string == NULL)
                return Steinberg::kInvalidArgument;

            size_t szof = (Steinberg::strlen16(string) + 1) * sizeof(Steinberg::Vst::TChar);
            return set_item(id, STRING, string, szof);
        }

        Steinberg::tresult PLUGIN_API Message::getString(AttrID id, Steinberg::Vst::TChar* string, Steinberg::uint32 sizeInBytes)
        {
            item_t *item = get_item(id, STRING);
            if (item == NULL)
                return Steinberg::kInvalidArgument;

            memcpy(string, item->data, lsp_min(item->size, sizeInBytes));

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Message::setBinary(AttrID id, const void *data, Steinberg::uint32 sizeInBytes)
        {
            if (data == NULL)
                return Steinberg::kInvalidArgument;

            return set_item(id, BLOB, data, sizeInBytes);
        }

        Steinberg::tresult PLUGIN_API Message::getBinary(AttrID id, const void *& data, Steinberg::uint32& sizeInBytes)
        {
            item_t *item = get_item(id, BLOB);
            if (item == NULL)
                return Steinberg::kInvalidArgument;

            data = item->data;
            sizeInBytes = item->size;

            return Steinberg::kResultOk;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_MESSAGE_H_ */

