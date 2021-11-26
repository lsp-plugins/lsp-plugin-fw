/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_EXTENSIONS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_EXTENSIONS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ext/osc.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/types.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/stdlib.h>
#include <lsp-plug.in/stdlib/string.h>

// LV2 includes
#include <lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/port-props/port-props.h>
#include <lv2/lv2plug.in/ns/ext/port-groups/port-groups.h>
#include <lv2/lv2plug.in/ns/ext/resize-port/resize-port.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/extensions/units/units.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/ext/instance-access/instance-access.h>

// Non-official features
#include <lsp-plug.in/3rdparty/ardour/inline-display.h>

// Some definitions that may be lacking in LV2
#ifndef LV2_ATOM__Object
    #define LV2_ATOM__Object            LV2_ATOM_PREFIX "Object"
#endif /* LV2_ATOM__Object */

#ifndef LV2_STATE__StateChanged
    #define LV2_STATE__StateChanged     LV2_STATE_PREFIX "StateChanged"
#endif /* LV2_STATE__StateChanged */

#pragma pack(push, 1)
typedef struct LV2_Atom_Midi
{
    LV2_Atom    atom;
    uint8_t     body[8];
} LV2_Atom_Midi;
#pragma pack(pop)

#define LV2PORT_MAX_BLOCK_LENGTH        8192

namespace lsp
{
    namespace lv2
    {
        struct Extensions;
        class Wrapper;

        /**
         * Serializable object
         */
        class Serializable
        {
            protected:
                lv2::Extensions        *pExt;
                LV2_URID                urid;

            public:
                explicit Serializable(Extensions *ext)
                {
                    pExt        = ext;
                    urid        = 0;
                }

                virtual ~Serializable()
                {
                }

            public:
                virtual void serialize() {};

                virtual void deserialize(const void *data) {};

            public:
                inline LV2_URID         get_urid() const        { return urid; };
        };

        /**
         * Set of bindings to LV2 extension interfaces.
         */
        struct Extensions
        {
            public:
                LV2_Atom_Forge          forge;

                // Extension interfaces
                LV2_URID_Map           *map;
                LV2_URID_Unmap         *unmap;
                LV2_Worker_Schedule    *sched;
                LV2_Inline_Display     *iDisplay;
                LV2_State_Map_Path     *mapPath;
                LV2UI_Resize           *ui_resize;
                lv2::Wrapper           *pWrapper;

                // State interface
                LV2_State_Store_Function    hStore;
                LV2_State_Retrieve_Function hRetrieve;
                LV2_State_Handle            hHandle;

                // Plugin URI and URID
                const char             *uriPlugin;
                const char             *uriTypes;
                const char             *uriKvt;
                LV2_URID                uridPlugin;

                // Miscellaneous URIDs
                LV2_URID                uridAtomTransfer;
                LV2_URID                uridEventTransfer;
                LV2_URID                uridObject;
                LV2_URID                uridBlank;
                LV2_URID                uridStateChanged;
                LV2_URID                uridUINotification;
                LV2_URID                uridConnectUI;
                LV2_URID                uridDisconnectUI;
                LV2_URID                uridDumpState;
                LV2_URID                uridPathType;
                LV2_URID                uridMidiEventType;
                LV2_URID                uridKvtKeys;
                LV2_URID                uridKvtObject;
                LV2_URID                uridKvtType;
                LV2_URID                uridKvtPropertyType;
                LV2_URID                uridKvtPropertyValue;
                LV2_URID                uridKvtPropertyFlags;
                LV2_URID                uridBlobType;
                LV2_URID                uridContentType;
                LV2_URID                uridContent;
                LV2_URID                uridTypeUInt;
                LV2_URID                uridTypeULong;

                LV2_URID                uridPatchGet;
                LV2_URID                uridPatchSet;
                LV2_URID                uridPatchMessage;
                LV2_URID                uridPatchProperty;
                LV2_URID                uridPatchValue;

                LV2_URID                uridAtomUrid;
                LV2_URID                uridChunk;
                LV2_URID                uridUpdateRate;

                LV2_URID                uridTimePosition;
                LV2_URID                uridTimeFrame;
                LV2_URID                uridTimeFrameRate;
                LV2_URID                uridTimeSpeed;
                LV2_URID                uridTimeBarBeat;
                LV2_URID                uridTimeBar;
                LV2_URID                uridTimeBeatUnit;
                LV2_URID                uridTimeBeatsPerBar;
                LV2_URID                uridTimeBeatsPerMinute;

                LV2_URID                uridMaxBlockLength;

                // OSC-related URIDs
                LV2_URID                uridOscBundle;
                LV2_URID                uridOscBundleTimetag;
                LV2_URID                uridOscBundleItems;
                LV2_URID                uridOscMessage;
                LV2_URID                uridOscMessagePath;
                LV2_URID                uridOscMessageArguments;
                LV2_URID                uridOscTimetag;
                LV2_URID                uridOscTimetagIntegral;
                LV2_URID                uridOscTimetagFraction;
                LV2_URID                uridOscNil;
                LV2_URID                uridOscImpulse;
                LV2_URID                uridOscChar;
                LV2_URID                uridOscRgba;
                LV2_URID                uridOscRawPacket;

                // LSP-related URIDs
                LV2_URID                uridMeshType;
                LV2_URID                uridMeshItems;
                LV2_URID                uridMeshDimensions;
                LV2_URID                uridMeshData;
                LV2_URID                uridFrameBufferType;
                LV2_URID                uridFrameBufferRows;        // Number of rows
                LV2_URID                uridFrameBufferCols;        // Number of cols
                LV2_URID                uridFrameBufferFirstRowID;  // First row identifier
                LV2_URID                uridFrameBufferLastRowID;   // Last row identifier
                LV2_URID                uridFrameBufferData;        // Frame buffer row data
                LV2_URID                uridStreamType;             // Stream data type
                LV2_URID                uridStreamDimensions;       // Stream dimensions
                LV2_URID                uridStreamFrame;            // Stream frame
                LV2_URID                uridStreamFrameType;        // Stream frame type
                LV2_URID                uridStreamFrameId;          // Number of frame
                LV2_URID                uridStreamFrameSize;        // Size of frame
                LV2_URID                uridStreamFrameData;        // Frame data

                LV2UI_Controller        ctl;
                LV2UI_Write_Function    wf;
                ssize_t                 nAtomIn;            // Atom input port identifier
                ssize_t                 nAtomOut;           // Atom output port identifier
                size_t                  nMaxBlockLength;    // Maximum size of audio block passed to plugin
                uint8_t                *pBuffer;            // Atom serialization buffer
                size_t                  nBufSize;           // Atom serialization buffer size
                float                   fUIRefreshRate;     // UI refresh rate
                void                   *pParentWindow;      // Parent window handle

            public:
                inline Extensions(
                    const LV2_Feature* const* feat,
                    const char *plugin_uri,
                    const char *types_uri,
                    const char *kvt_uri,
                    LV2UI_Controller lv2_ctl,
                    LV2UI_Write_Function lv2_write)
                {
                    map                 = NULL;
                    unmap               = NULL;
                    ui_resize           = NULL;
                    sched               = NULL;
                    iDisplay            = NULL;
                    mapPath             = NULL;
                    pParentWindow       = NULL;
                    pWrapper            = NULL;
                    fUIRefreshRate      = MESH_REFRESH_RATE;
                    nMaxBlockLength     = LV2PORT_MAX_BLOCK_LENGTH;

                    const LV2_Options_Option *opts = NULL;

                    // Scan features
                    if (feat != NULL)
                    {
                        for (size_t i=0; feat[i]; ++i)
                        {
                            const LV2_Feature *f = feat[i];

                            lsp_trace("Host reported extension uri=%s, data=%p", f->URI, f->data);

                            if (!strcmp(f->URI, LV2_URID__map))
                                map = reinterpret_cast<LV2_URID_Map *>(f->data);
                            else if (!strcmp(f->URI, LV2_URID__unmap))
                                unmap = reinterpret_cast<LV2_URID_Unmap *>(f->data);
                            else if (!strcmp(f->URI, LV2_WORKER__schedule))
                                sched = reinterpret_cast<LV2_Worker_Schedule *>(f->data);
                            else if (!strcmp(f->URI, LV2_UI__parent))
                                pParentWindow   = f->data;
                            else if (!strcmp(f->URI, LV2_UI__resize))
                                ui_resize = reinterpret_cast<LV2UI_Resize *>(f->data);
                            else if (!strcmp(f->URI, LV2_INLINEDISPLAY__queue_draw))
                                iDisplay = reinterpret_cast<LV2_Inline_Display *>(f->data);
                        #if LSP_LV2_NO_INSTANCE_ACCESS != 1
                            else if (!strcmp(f->URI, LV2_INSTANCE_ACCESS_URI))
                                pWrapper = reinterpret_cast<Wrapper *>(f->data);
                        #endif
                            else if (!strcmp(f->URI, LV2_OPTIONS__options))
                            {
                                lsp_trace("Received options from host");
                                opts = reinterpret_cast<const LV2_Options_Option *>(f->data);
                            }
                        }

                        lsp_trace("Plugin instance wrapper pointer: %p", pWrapper);
                    }

                    // Initialize basic URIDs
                    ctl                         = lv2_ctl;
                    wf                          = lv2_write;
                    nAtomIn                     = -1;
                    nAtomOut                    = -1;
                    pBuffer                     = NULL;
                    nBufSize                    = 0;

                    uriPlugin                   = plugin_uri;
                    uriTypes                    = types_uri;
                    uriKvt                      = kvt_uri;
                    uridPlugin                  = (map != NULL) ? map->map(map->handle, uriPlugin) : -1;

                    if (map != NULL)
                        lv2_atom_forge_init(&forge, map);

                    uridAtomTransfer            = map_uri(LV2_ATOM__atomTransfer);
                    uridEventTransfer           = map_uri(LV2_ATOM__eventTransfer);
                    uridObject                  = forge.Object;
                    uridBlank                   = map_uri(LV2_ATOM__Blank);
                    uridStateChanged            = map_uri(LV2_STATE__StateChanged);
                    uridUINotification          = map_type_legacy("UINotification");
                    uridConnectUI               = map_primitive("ui_connect");
                    uridDisconnectUI            = map_primitive("ui_disconnect");
                    uridDumpState               = map_primitive("dumpState");
                    uridPathType                = forge.Path;
                    uridMidiEventType           = map_uri(LV2_MIDI__MidiEvent);
                    uridKvtObject               = map_primitive("KVT");
                    uridKvtType                 = map_type_legacy("KVT");
                    uridKvtPropertyType         = map_type_legacy("KVTProperty");
                    uridKvtPropertyValue        = map_field("KVTProperty", "value");
                    uridKvtPropertyFlags        = map_field("KVTProperty", "flags");
                    uridBlobType                = map_type_legacy("Blob");
                    uridContentType             = map_field("Blob", "ContentType");
                    uridContent                 = map_field("Blob", "Content");

                    uridTypeUInt                = map_uri(LV2_ATOM_PREFIX "UInt" );
                    uridTypeULong               = map_uri(LV2_ATOM_PREFIX "ULong" );

                    uridPatchGet                = map_uri(LV2_PATCH__Get);
                    uridPatchSet                = map_uri(LV2_PATCH__Set);
                    uridPatchMessage            = map_uri(LV2_PATCH__Message);
                    uridPatchProperty           = map_uri(LV2_PATCH__property);
                    uridPatchValue              = map_uri(LV2_PATCH__value);
                    uridAtomUrid                = forge.URID;
                    uridChunk                   = forge.Chunk;
                    uridUpdateRate              = map_uri(LV2_UI__updateRate);

                    uridTimePosition            = map_uri(LV2_TIME__Position);
                    uridTimeFrame               = map_uri(LV2_TIME__frame);
                    uridTimeFrameRate           = map_uri(LV2_TIME__framesPerSecond);
                    uridTimeSpeed               = map_uri(LV2_TIME__speed);
                    uridTimeBarBeat             = map_uri(LV2_TIME__barBeat);
                    uridTimeBar                 = map_uri(LV2_TIME__bar);
                    uridTimeBeatUnit            = map_uri(LV2_TIME__beatUnit);
                    uridTimeBeatsPerBar         = map_uri(LV2_TIME__beatsPerBar);
                    uridTimeBeatsPerMinute      = map_uri(LV2_TIME__beatsPerMinute);

                    uridMaxBlockLength          = map_uri(LV2_BUF_SIZE__maxBlockLength);

                    // OSC-related URIDs
                    uridOscBundle               = map_uri(LV2_OSC__Bundle);
                    uridOscBundleTimetag        = map_uri(LV2_OSC__bundleTimetag);
                    uridOscBundleItems          = map_uri(LV2_OSC__bundleItems);
                    uridOscMessage              = map_uri(LV2_OSC__Message);
                    uridOscMessagePath          = map_uri(LV2_OSC__messagePath);
                    uridOscMessageArguments     = map_uri(LV2_OSC__messageArguments);
                    uridOscTimetag              = map_uri(LV2_OSC__Timetag);
                    uridOscTimetagIntegral      = map_uri(LV2_OSC__timetagIntegral);
                    uridOscTimetagFraction      = map_uri(LV2_OSC__timetagFraction);
                    uridOscNil                  = map_uri(LV2_OSC__Nil);
                    uridOscImpulse              = map_uri(LV2_OSC__Impulse);
                    uridOscChar                 = map_uri(LV2_OSC__Char);
                    uridOscRgba                 = map_uri(LV2_OSC__RGBA);
                    uridOscRawPacket            = map_uri(LV2_OSC__RawPacket);

                    // LSP-related URIDs
                    uridMeshType                = map_type_legacy("Mesh");
                    uridMeshItems               = map_field("Mesh", "items");
                    uridMeshDimensions          = map_field("Mesh", "dimensions");
                    uridMeshData                = map_field("Mesh", "data");

                    uridFrameBufferType         = map_type_legacy("FrameBuffer");
                    uridFrameBufferRows         = map_field("FrameBuffer", "rows");
                    uridFrameBufferCols         = map_field("FrameBuffer", "columns");
                    uridFrameBufferFirstRowID   = map_field("FrameBuffer", "firstRowID");
                    uridFrameBufferLastRowID    = map_field("FrameBuffer", "lastRowID");
                    uridFrameBufferData         = map_field("FrameBuffer", "data");

                    uridStreamType              = map_type_legacy("Stream");
                    uridStreamDimensions        = map_field("Stream", "dimensions");
                    uridStreamFrame             = map_field("Stream", "frame");
                    uridStreamFrameType         = map_type_legacy("StreamFrame");
                    uridStreamFrameId           = map_field("StreamFrame", "id");
                    uridStreamFrameSize         = map_field("StreamFrame", "size");
                    uridStreamFrameData         = map_field("StreamFrame", "data");

                    // Decode passed options if they are present
                    if (opts != NULL)
                    {
                        lsp_trace("Decoding passed options");
                        while ((opts->key != 0) && (opts->value != 0))
                        {
                            lsp_trace("context = %d (%s), subject=%d, key = %d (%s), size = %d, type=%d (%s), value=%p",
                                    int(opts->context),
                                    (opts->context == LV2_OPTIONS_INSTANCE) ? "instance" :
                                    (opts->context == LV2_OPTIONS_RESOURCE) ? "resource" :
                                    (opts->context == LV2_OPTIONS_BLANK)    ? "blank" :
                                    (opts->context == LV2_OPTIONS_PORT)     ? "port" : "unknown",
                                    int(opts->subject),
                                    int(opts->key), unmap_urid(opts->key),
                                    int(opts->size),
                                    int(opts->type), unmap_urid(opts->type),
                                    opts->value
                            );

                            // Update Rate for the UI
                            if ((opts->context == LV2_OPTIONS_INSTANCE) &&
                                (opts->key == uridUpdateRate) &&
                                (opts->value != NULL))
                            {
                                if ((opts->type == forge.Float) && (opts->size == sizeof(float)))
                                    fUIRefreshRate  = *reinterpret_cast<const float *>(opts->value);
                                else if ((opts->type == forge.Double) && (opts->size == sizeof(double)))
                                    fUIRefreshRate  = *reinterpret_cast<const double *>(opts->value);
                                else if ((opts->type == forge.Int) && (opts->size == sizeof(int32_t)))
                                    fUIRefreshRate  = *reinterpret_cast<const int32_t *>(opts->value);
                                else if ((opts->type == forge.Long) && (opts->size == sizeof(int64_t)))
                                    fUIRefreshRate  = *reinterpret_cast<const int64_t *>(opts->value);
                                if (fUIRefreshRate < 0)
                                    fUIRefreshRate = MESH_REFRESH_RATE;
                                lsp_trace("UI refresh rate has been set to %f", fUIRefreshRate);
                            }

                            if ((opts->context == LV2_OPTIONS_INSTANCE) &&
                                (opts->key == uridMaxBlockLength) &&
                                (opts->value != NULL))
                            {
                                ssize_t blk_len = nMaxBlockLength;
                                if ((opts->type == forge.Int) && (opts->size == sizeof(int32_t)))
                                    blk_len = *reinterpret_cast<const int32_t *>(opts->value);
                                else if ((opts->type == forge.Long) && (opts->size == sizeof(int64_t)))
                                    blk_len = *reinterpret_cast<const int64_t *>(opts->value);
                                if (blk_len > 0)
                                    nMaxBlockLength = blk_len;
                                lsp_trace("MaxBlockLength has been set to %d", int(nMaxBlockLength));
                            }

                            opts++;
                        }
                    }
                }

                ~Extensions()
                {
                    lsp_trace("destroy");

                    // Drop atom buffer
                    if (pBuffer != NULL)
                    {
                        delete [] pBuffer;
                        pBuffer     = NULL;
                    }
                }

            public:
                inline float ui_refresh_rate() const
                {
                    return fUIRefreshRate;
                }

                inline void *parent_window()
                {
                    return pParentWindow;
                }

                inline bool atom_supported() const
                {
                    return map != NULL;
                }

                inline const char *unmap_urid(LV2_URID urid)
                {
                    return (unmap != NULL) ? unmap->unmap(unmap->handle, urid) : NULL;
                }

                inline lv2::Wrapper *wrapper()
                {
                    return pWrapper;
                }

                inline void write_data(
                        uint32_t         port_index,
                        uint32_t         buffer_size,
                        uint32_t         port_protocol,
                        const void*      buffer)
                {
                    if ((ctl == NULL) || (wf == NULL))
                        return;
                    wf(ctl, port_index, buffer_size, port_protocol, buffer);
                }

                inline LV2_Atom *forge_object(LV2_Atom_Forge_Frame* frame, LV2_URID id, LV2_URID otype)
                {
                    const LV2_Atom_Object a = {
                        { sizeof(LV2_Atom_Object_Body), uridObject },
                        { id, otype }
                    };
                    return reinterpret_cast<LV2_Atom *>(
                        lv2_atom_forge_push(&forge, frame, lv2_atom_forge_write(&forge, &a, sizeof(a)))
                    );
                }

                inline LV2_Atom_Vector *forge_vector(
                        uint32_t        child_size,
                        uint32_t        child_type,
                        uint32_t        n_elems,
                        const void*     elems)
                {
                    return reinterpret_cast<LV2_Atom_Vector *>(
                        lv2_atom_forge_vector(&forge, child_size, child_type, n_elems, elems)
                    );
                }

                inline LV2_Atom_Forge_Ref forge_sequence_head(LV2_Atom_Forge_Frame* frame, uint32_t unit)
                {
                    return lv2_atom_forge_sequence_head(&forge, frame, unit);
                }

                inline LV2_Atom_Forge_Ref forge_key(LV2_URID key)
                {
                    const uint32_t body[] = { key, 0 };
                    return lv2_atom_forge_write(&forge, body, sizeof(body));
                }

                inline LV2_Atom_Forge_Ref forge_urid(LV2_URID urid)
                {
                    const LV2_Atom_URID a = { { sizeof(LV2_URID), forge.URID }, urid };
                    return lv2_atom_forge_write(&forge, &a, sizeof(LV2_Atom_URID));
                }

                inline LV2_Atom_Forge_Ref forge_path(const char *str)
                {
                    size_t len = ::strlen(str);
                    return lv2_atom_forge_typed_string(&forge, forge.Path, str, len);
                }

                inline LV2_Atom_Forge_Ref forge_string(const char *str)
                {
                    size_t len = ::strlen(str);
                    return lv2_atom_forge_typed_string(&forge, forge.String, str, len);
                }

                inline void forge_pop(LV2_Atom_Forge_Frame* frame)
                {
                    lv2_atom_forge_pop(&forge, frame);
                }

                inline LV2_Atom_Forge_Ref forge_float(float val)
                {
                    const LV2_Atom_Float a = { { sizeof(float), forge.Float }, val };
                    return lv2_atom_forge_primitive(&forge, &a.atom);
                }

                inline LV2_Atom_Forge_Ref forge_int(int32_t val)
                {
                    const LV2_Atom_Int a    = { { sizeof(int32_t), forge.Int }, val };
                    return lv2_atom_forge_primitive(&forge, &a.atom);
                }

                inline LV2_Atom_Forge_Ref forge_long(int64_t val)
                {
                    const LV2_Atom_Long a = { { sizeof(val), forge.Long }, val };
                    return lv2_atom_forge_primitive(&forge, &a.atom);
                }

                inline void forge_pad(size_t size)
                {
                    lv2_atom_forge_pad(&forge, size);
                }

                inline LV2_Atom_Forge_Ref forge_raw(const void* data, size_t size)
                {
                    return lv2_atom_forge_raw(&forge, data, size);
                }

                inline LV2_Atom_Forge_Ref forge_primitive(const LV2_Atom *atom)
                {
                    return lv2_atom_forge_primitive(&forge, atom);
                }

                inline void forge_set_buffer(void* buf, size_t size)
                {
                    lv2_atom_forge_set_buffer(&forge, reinterpret_cast<uint8_t *>(buf), size);
                }

                inline LV2_Atom_Forge_Ref forge_frame_time(int64_t frames)
                {
                    return lv2_atom_forge_write(&forge, &frames, sizeof(frames));
                }

                inline void resize_ui(ssize_t width, ssize_t height)
                {
                    if (ui_resize != NULL)
                        ui_resize->ui_resize(ui_resize->handle, width, height);
                }

                inline LV2_URID map_uri(const char *fmt...) const
                {
                    if (map == NULL)
                        return -1;

                    va_list vl;
                    char tmpbuf[2048];

                    va_start(vl, fmt);
                    vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, vl);
                    va_end(vl);

                    LV2_URID res = map->map(map->handle, tmpbuf);
                    lsp_trace("URID for <%s> is %d (0x%x)", tmpbuf, int(res), int(res));
                    return res;
                }

                inline LV2_URID map_port(const char *id) const
                {
                    return map_uri("%s/ports#%s", uriPlugin, id);
                }

                inline LV2_URID map_type_legacy(const char *type) const
                {
                    return map_uri("%s/types#%s", uriTypes, type);
                }

                inline LV2_URID map_type(const char *type) const
                {
                    return map_uri("%s/%s", uriTypes, type);
                }

                inline LV2_URID map_field(const char *type, const char *id) const
                {
                    return map_uri("%s/%s#%s", uriTypes, type, id);
                }

                inline LV2_URID map_kvt(const char *id) const
                {
                    return map_uri("%s/%s", uriKvt, id);
                }

                inline LV2_URID map_primitive(const char *id) const
                {
                    return map_uri("%s/%s", uriPlugin, id);
                }

                inline void init_state_context(
                    LV2_State_Store_Function    store,
                    LV2_State_Retrieve_Function retrieve,
                    LV2_State_Handle            handle,
                    uint32_t                    flags,
                    const LV2_Feature *const *  features
                )
                {
                    hStore          = store;
                    hRetrieve       = retrieve;
                    hHandle         = handle;

                    for (size_t i=0; features[i]; ++i)
                    {
                        const LV2_Feature *f = features[i];
                        lsp_trace("Host reported state extension uri=%s, data=%p", f->URI, f->data);

                        if (!::strcmp(f->URI, LV2_STATE__mapPath))
                            mapPath = reinterpret_cast<LV2_State_Map_Path *>(f->data);
                    }
                }

                inline void store_value(LV2_URID urid, LV2_URID type, const void *data, size_t size)
                {
                    if ((hStore == NULL) || (hHandle == NULL))
                        return;
                    hStore(hHandle, urid, data, size, type, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
                }

                inline const void *retrieve_value(LV2_URID urid, uint32_t *type, size_t *size)
                {
                    if ((hRetrieve == NULL) || (hHandle == NULL))
                        return NULL;

                    uint32_t t_flags;
                    lsp_trace("retrieve %d (%s)", urid, unmap_urid(urid));
                    const void *ptr   = hRetrieve(hHandle, urid, size, type, &t_flags);
                    lsp_trace("retrieved ptr = %p, size=%d, type=%d, flags=0x%x", ptr, int(*size), int(*type), int(t_flags));

                    return ptr;
                }

                inline const void *restore_value(LV2_URID urid, LV2_URID type, size_t *size)
                {
                    uint32_t t_type;
                    size_t t_size;
                    const void *ptr = retrieve_value(urid, &t_type, &t_size);
                    if (t_type != type)
                        return NULL;

                    lsp_trace("retrieved type = %d (%s)", int(t_type), unmap_urid(t_type));
                    *size    = t_size;
                    return ptr;
                }

                inline void reset_state_context()
                {
                    hStore          = NULL;
                    hRetrieve       = NULL;
                    hHandle         = NULL;
                    mapPath         = NULL;
                }

                inline bool ui_create_atom_transport(size_t port, size_t buf_size)
                {
                    // Remember IDs of atom ports
                    nAtomOut    = port++;
                    nAtomIn     = port;

                    // Allocate buffer
                    nBufSize    = buf_size;
                    pBuffer     = new uint8_t[nBufSize];
                    if (pBuffer == NULL)
                        return false;

                    lsp_trace("Atom rx_id=%d, tx_id=%d, buf_size=%d, buffer=%p", int(nAtomIn), int(nAtomOut), int(nBufSize), pBuffer);
                    return true;
                }

                inline bool ui_connect_to_plugin()
                {
                    if (map == NULL)
                        return false;
                    if (pWrapper != NULL)
                        return true;

                    // Prepare forge for transfer
                    LV2_Atom_Forge_Frame    frame;
                    forge_set_buffer(pBuffer, nBufSize);

                    // Send CONNECT UI message
                    lsp_trace("Sending CONNECT UI message");
                    LV2_Atom *msg = forge_object(&frame, uridConnectUI, uridUINotification);
                    forge_pop(&frame);
                    write_data(nAtomOut, lv2_atom_total_size(msg), uridEventTransfer, msg);

                    return true;
                }

                inline bool request_state_dump()
                {
                    if (map == NULL)
                        return false;

                    // Prepare forge for transfer
                    LV2_Atom_Forge_Frame    frame;
                    forge_set_buffer(pBuffer, nBufSize);

                    // Send DUMP STATE message
                    lsp_trace("Sending DUMP STATE message");
                    LV2_Atom *msg = forge_object(&frame, uridDumpState, uridUINotification);
                    forge_pop(&frame);
                    write_data(nAtomOut, lv2_atom_total_size(msg), uridEventTransfer, msg);

                    return true;
                }

                inline void ui_disconnect_from_plugin()
                {
                    if (map == NULL)
                        return;
                    if (pWrapper != NULL)
                        return;

                    // Prepare ofrge for transfer
                    LV2_Atom_Forge_Frame    frame;
                    forge_set_buffer(pBuffer, nBufSize);

                    // Send DISCONNECT UI message
                    lsp_trace("Sending DISCONNECT UI message");
                    LV2_Atom *msg = forge_object(&frame, uridDisconnectUI, uridUINotification);
                    forge_pop(&frame);
                    write_data(nAtomOut, lv2_atom_total_size(msg), uridEventTransfer, msg);
                }

                bool ui_write_patch(lv2::Serializable *p)
                {
                    if ((map == NULL) || (p->get_urid() <= 0))
                        return false;

                    // Forge PATCH SET message
                    LV2_Atom_Forge_Frame    frame;
                    forge_set_buffer(pBuffer, nBufSize);

                    forge_frame_time(0);
                    LV2_Atom *msg = forge_object(&frame, uridChunk, uridPatchSet);
                    forge_key(uridPatchProperty);
                    forge_urid(p->get_urid());
                    forge_key(uridPatchValue);
                    p->serialize();
                    forge_pop(&frame);

                    write_data(nAtomOut, lv2_atom_total_size(msg), uridEventTransfer, msg);
                    return true;
                }
        };

        #define PATCH_OVERHEAD  (sizeof(LV2_Atom_Property) + sizeof(LV2_Atom_URID) + sizeof(LV2_Atom) + 0x20)

        inline long lv2_all_port_sizes(const meta::port_t *ports, bool in, bool out)
        {
            long size           = 0;

            for (const meta::port_t *p = ports; p->id != NULL; ++p)
            {
                switch (p->role)
                {
                    case meta::R_CONTROL:
                    case meta::R_METER:
                        size            += PATCH_OVERHEAD + sizeof(LV2_Atom_Float);
                        break;
                    case meta::R_MESH:
                        if (meta::is_out_port(p) && (!out))
                            break;
                        else if (meta::is_in_port(p) && (!in))
                            break;
                        size            += lv2_mesh_t::size_of_port(p);
                        break;
                    case meta::R_STREAM:
                    {
                        if (meta::is_out_port(p) && (!out))
                            break;
                        else if (meta::is_in_port(p) && (!in))
                            break;

                        size_t vector_len   = sizeof(LV2_Atom_Vector) + 4 * sizeof(LV2_Atom_Int) + sizeof(float) * STREAM_MAX_FRAME_SIZE;
                        size_t frm_size     = sizeof(LV2_Atom_Object) + 8 * sizeof(LV2_Atom_Int) + size_t(p->min) * vector_len;
                        size_t data_size    = sizeof(LV2_Atom_Object) + 8 * sizeof(LV2_Atom_Int) + STREAM_BULK_MAX * frm_size;
                        size               += data_size;
                        break;
                    }
                    case meta::R_FBUFFER:
                        if (meta::is_out_port(p) && (!out))
                            break;
                        else if (meta::is_in_port(p) && (!in))
                            break;
                        size           += (4 * sizeof(LV2_Atom_Int) + 0x100) + // Headers
                                            size_t(p->step) * FRAMEBUFFER_BULK_MAX * sizeof(float);
                        break;
                    case meta::R_OSC:
                        size           += OSC_BUFFER_MAX;
                        break;
                    case meta::R_MIDI:
                        if (meta::is_out_port(p) && (!out))
                            break;
                        else if (meta::is_in_port(p) && (!in))
                            break;
                        size            += (sizeof(LV2_Atom_Event) + 0x10) * MIDI_EVENTS_MAX; // Size of atom event + pad for MIDI data
                        break;
                    case meta::R_PATH: // Both sizes: IN and OUT
                        size            += PATCH_OVERHEAD + PATH_MAX;
                        break;
                    case meta::R_PORT_SET:
                        if ((p->members != NULL) && (p->items != NULL))
                        {
                            size_t items        = list_size(p->items);
                            size               += items * lv2_all_port_sizes(p->members, in, out); // Add some overhead
                            size               += sizeof(LV2_Atom_Int) + 0x10;
                        }
                        break;
                    default:
                        break;
                }
            }

            // Update state size
            return LSP_LV2_SIZE_PAD(size); // Add some extra bytes for
        }

        #undef PATCH_OVERHEAD
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_EXTENSIONS_H_ */
