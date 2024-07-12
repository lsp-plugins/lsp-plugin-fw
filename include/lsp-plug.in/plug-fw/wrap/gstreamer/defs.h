/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 июн. 2024 г.
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


#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_DEFS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_DEFS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

//-----------------------------------------------------------------------------
// Workaround for rare GStreamer library begin
G_BEGIN_DECLS

#ifndef _GST_ELEMENT_REGISTER_DEFINE_BEGIN
    #define _GST_ELEMENT_REGISTER_DEFINE_BEGIN(element) \
    G_BEGIN_DECLS \
    gboolean G_PASTE (gst_element_register_, element) (GstPlugin * plugin) \
    { \
      {
#endif /* _GST_ELEMENT_REGISTER_DEFINE_BEGIN */

#ifndef _GST_ELEMENT_REGISTER_DEFINE_END
    #define _GST_ELEMENT_REGISTER_DEFINE_END(element_name, rank, type) \
      } \
      return gst_element_register (plugin, element_name, rank, type); \
    } \
    G_END_DECLS
#endif /* _GST_ELEMENT_REGISTER_DEFINE_END */

#ifndef GST_ELEMENT_REGISTER_DEFINE_CUSTOM
    #define GST_ELEMENT_REGISTER_DEFINE_CUSTOM(element, register_func) \
    G_BEGIN_DECLS \
    gboolean G_PASTE (gst_element_register_, element) (GstPlugin * plugin) \
    { \
      return register_func (plugin); \
    } \
    G_END_DECLS
#endif /* GST_ELEMENT_REGISTER_DEFINE_CUSTOM */

#ifndef GST_ELEMENT_REGISTER_DEFINE
    #define GST_ELEMENT_REGISTER_DEFINE(e, e_n, r, t) _GST_ELEMENT_REGISTER_DEFINE_BEGIN(e) _GST_ELEMENT_REGISTER_DEFINE_END(e_n, r, t)
#endif /* GST_ELEMENT_REGISTER_DEFINE */

#ifndef GST_ELEMENT_REGISTER_DEFINE_WITH_CODE
    #define GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(e, e_n, r, t, _c_) _GST_ELEMENT_REGISTER_DEFINE_BEGIN(e) {_c_;} _GST_ELEMENT_REGISTER_DEFINE_END(e_n, r, t)
#endif /* GST_ELEMENT_REGISTER_DEFINE_WITH_CODE */

#ifndef GST_ELEMENT_REGISTER_DECLARE
    #define GST_ELEMENT_REGISTER_DECLARE(element) \
    G_BEGIN_DECLS \
    gboolean G_PASTE(gst_element_register_, element) (GstPlugin * plugin); \
    G_END_DECLS
#endif /* GST_ELEMENT_REGISTER_DECLARE */

#ifndef GST_ELEMENT_REGISTER
    #define GST_ELEMENT_REGISTER(element, plugin) G_PASTE(gst_element_register_, element) (plugin)
#endif /* GST_ELEMENT_REGISTER */

G_END_DECLS

// Workaround for rare GStreamer library end
//-----------------------------------------------------------------------------


#define GSTREAMER_FACTORY_FUNCTION          plug_fw_get_gst_factory
#define GSTREAMER_FACTORY_FUNCTION_NAME     LSP_STRINGIFY(GSTREAMER_FACTORY_FUNCTION)

namespace lsp
{
    namespace gst
    {
        /**
         * Acquire some resource
         * @tparam T type of resource
         * @param ptr poninter to resource
         * @return the passed pointer
         */
        template <class T>
        inline T * safe_acquire(T *ptr)
        {
            if (ptr != NULL)
                ptr->acquire();
            return ptr;
        }

        /**
         * Release some resource
         * @tparam T type of resource
         * @param ptr pointer to the resource
         */
        template <class T>
        inline void safe_release(T * &ptr)
        {
            if (ptr == NULL)
                return;

            ptr->release();
            ptr = NULL;
        }

        // Wrapper interface
        struct IWrapper
        {
            virtual ~IWrapper();
            virtual void setup(const GstAudioInfo * info) = 0;
            virtual void change_state(GstStateChange transition) = 0;
            virtual void set_property(guint prop_id, const GValue *value, GParamSpec *pspec) = 0;
            virtual void get_property(guint prop_id, GValue *value, GParamSpec *pspec) = 0;
            virtual gboolean query(GstPad *pad, GstQuery *query) = 0;
            virtual void process(guint8 *out, const guint8 *in, size_t out_size, size_t in_size) = 0;
        };

        // Factory interface
        struct IFactory
        {
            virtual ~IFactory();
            virtual status_t        init() = 0;
            virtual void            init_class(GstAudioFilterClass *klass, const char *plugin_id) = 0;
            virtual gst::IWrapper  *instantiate(const char *plugin_id, GstAudioFilter *filter) = 0;
        };

        typedef IFactory *(* factory_function_t)();

    } /* namespace gst */
} /* namespace lsp */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    LSP_EXPORT_MODIFIER
    lsp::gst::IFactory *GSTREAMER_FACTORY_FUNCTION();

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_DEFS_H_ */
