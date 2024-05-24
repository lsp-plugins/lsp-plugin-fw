/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 мая 2024 г.
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

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/finally.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/wrapper.h>

// Define plugin metadata
/* PLUGIN_METADATA_DEFINITIONS */

// Definitions of plugin metadata parameters to make IDE not frustrating with names
#ifdef LSP_IDE_DEBUG
    #ifndef DEF_PLUGIN_PACKAGE_ID
        #define DEF_PLUGIN_PACKAGE_ID           "plugin_template_package"
    #endif /* DEF_PLUGIN_PACKAGE_ID */

    #ifndef DEF_PLUGIN_PACKAGE_NAME
        #define DEF_PLUGIN_PACKAGE_NAME         "Plugin Template Package"
    #endif /* DEF_PLUGIN_PACKAGE_NAME */

    #ifndef DEF_PLUGIN_ID
        #define DEF_PLUGIN_ID                   "plugin_template"
    #endif /* DEF_PLUGIN_TYPE */

    #ifndef DEF_PLUGIN_NAME
        #define DEF_PLUGIN_NAME                 "Plugin Template"
    #endif /* DEF_PLUGIN_NAME */

    #ifndef DEF_PLUGIN_VERSION_STR
        #define DEF_PLUGIN_VERSION_STR          "0.0.0"
    #endif /* DEF_PLUGIN_VERSION_STR */

    #ifndef DEF_PLUGIN_LICENSE
        #define DEF_PLUGIN_LICENSE              ""
    #endif /* DEF_PLUGIN_LICENSE */

    #ifndef DEF_PLUGIN_ORIGIN
        #define DEF_PLUGIN_ORIGIN ""
    #endif /* DEF_PLUGIN_ORIGIN */

#endif /* LSP_IDE_DEBUG */

#define gst_xx_plugin_id_xx_parent_class parent_class
#define GST_TYPE_XX_PLUGIN_ID_XX (gst_xx_plugin_id_xx_get_type())
#ifndef PACKAGE
    #define PACKAGE     DEF_PLUGIN_PACKAGE_ID
#endif /* PACKAGE */

G_DECLARE_FINAL_TYPE( // @suppress("Unused static function")
    GstXx_PluginId_Xx,
    gst_xx_plugin_id_xx,
    GST,
    XX_PLUGIN_ID_XX,
    GstAudioFilter);

struct _GstXx_PluginId_Xx
{
    GstAudioFilter audiofilter;
    lsp::gst::Wrapper *wrapper;
};

G_DEFINE_TYPE( // @suppress("Unused static function")
    GstXx_PluginId_Xx,
    gst_xx_plugin_id_xx,
    GST_TYPE_AUDIO_FILTER);

GST_ELEMENT_REGISTER_DEFINE(
    xx_plugin_id_xx,
    DEF_PLUGIN_ID,
    GST_RANK_NONE,
    GST_TYPE_XX_PLUGIN_ID_XX);


static void gst_xx_plugin_id_xx_set_property(
    GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
    GstXx_PluginId_Xx *filter = GST_XX_PLUGIN_ID_XX(object);

    GST_OBJECT_LOCK(filter);
    lsp_finally { GST_OBJECT_UNLOCK(filter); };

    // TODO
}

static void gst_xx_plugin_id_xx_get_property(
    GObject * object,
    guint prop_id,
    GValue * value,
    GParamSpec * pspec)
{
    GstXx_PluginId_Xx *filter = GST_XX_PLUGIN_ID_XX(object);

    GST_OBJECT_LOCK(filter);
    lsp_finally { GST_OBJECT_UNLOCK(filter); };

    // TODO
}

static gboolean gst_xx_plugin_id_xx_setup(
    GstAudioFilter * object,
    const GstAudioInfo * info)
{
    GstAudioFilterClass *audio_filter_class = GST_AUDIO_FILTER_CLASS(parent_class);
//    GstXx_PluginId_Xx *filter = GST_XX_PLUGIN_ID_XX(object);
//
//    gint sample_rate = GST_AUDIO_INFO_RATE(info);
//    IF_TRACE(
//        gint channels = GST_AUDIO_INFO_CHANNELS(info);
//        GstAudioFormat fmt = GST_AUDIO_INFO_FORMAT(info);
//    );

    // Update sample rate
    // TODO

    return (audio_filter_class->setup) ?
        audio_filter_class->setup(object, info) :
        TRUE;
}

static GstFlowReturn gst_xx_plugin_id_xx_filter(
    GstBaseTransform *object,
    GstBuffer *inbuf,
    GstBuffer *outbuf)
{
//    GstXx_PluginId_Xx *filter = GST_XX_PLUGIN_ID_XX(object);

    // Map buffers
    GstMapInfo map_in;
    if (!gst_buffer_map (inbuf, &map_in, GST_MAP_READ))
        return GST_FLOW_OK;
    lsp_finally { gst_buffer_unmap (inbuf, &map_in); };

    GstMapInfo map_out;
    if (!gst_buffer_map (outbuf, &map_out, GST_MAP_WRITE))
        return GST_FLOW_OK;
    lsp_finally { gst_buffer_unmap (outbuf, &map_out); };

    g_assert (map_out.size == map_in.size);

    // Call processing
//    gst_xx_plugin_id_xx_process(filter, map_out.data, map_in.data, map_out.size);
    return GST_FLOW_OK;
}

static GstFlowReturn gst_xx_plugin_id_xx_filter_inplace(
    GstBaseTransform *object,
    GstBuffer *buf)
{
//    GstXx_PluginId_Xx *filter = GST_XX_PLUGIN_ID_XX(object);

    // Map buffer
    GstMapInfo map;
    if (!gst_buffer_map (buf, &map, GST_MAP_READWRITE))
        return GST_FLOW_OK;
    lsp_finally { gst_buffer_unmap (buf, &map); };

    // Call processing
//    gst_xx_plugin_id_xx_process(filter, map.data, map.data, map.size);
    return GST_FLOW_OK;
}

static void gst_xx_plugin_id_xx_init(GstXx_PluginId_Xx *filter)
{
    // TODO
}

static void gst_xx_plugin_id_xx_finalize(GObject * object)
{
//    GstXx_PluginId_Xx *filter = GST_XX_PLUGIN_ID_XX(object);

    // TODO

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

// GObject vmethod implementations
static void gst_xx_plugin_id_xx_class_init(GstXx_PluginId_XxClass * klass)
{
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
//    GstElementClass *element_class = reinterpret_cast<GstElementClass *>(klass); // TODO
    GstBaseTransformClass *btrans_class = reinterpret_cast<GstBaseTransformClass *>(klass);
    GstAudioFilterClass *audio_filter_class = reinterpret_cast<GstAudioFilterClass *>(klass);

    gobject_class->set_property = gst_xx_plugin_id_xx_set_property;
    gobject_class->get_property = gst_xx_plugin_id_xx_get_property;
    gobject_class->finalize = gst_xx_plugin_id_xx_finalize;

    // this function will be called when the format is set before the
    // first buffer comes in, and whenever the format changes
    audio_filter_class->setup = gst_xx_plugin_id_xx_setup;

    // here you set up functions to process data (either in place, or from
    // one input buffer to another output buffer); only one is required
    btrans_class->transform = gst_xx_plugin_id_xx_filter;
    btrans_class->transform_ip = gst_xx_plugin_id_xx_filter_inplace;

    // TODO
}

static gboolean plugin_init(GstPlugin *plugin)
{
    return GST_ELEMENT_REGISTER(xx_plugin_id_xx, plugin);
}

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    xx_plugin_id_xx,
    DEF_PLUGIN_NAME,
    plugin_init,
    DEF_PLUGIN_VERSION_STR,
    DEF_PLUGIN_LICENSE,
    DEF_PLUGIN_PACKAGE_NAME,
    DEF_PLUGIN_ORIGIN);

