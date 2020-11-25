/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/plug-fw/core/JsonDumper.h>

namespace lsp
{
    namespace plug
    {
        IWrapper::IWrapper(IModule *plugin)
        {
            pPlugin     = plugin;
        }

        IWrapper::~IWrapper()
        {
        }

        ipc::IExecutor *IWrapper::get_executor()
        {
            return NULL;
        }

        void IWrapper::query_display_draw()
        {
        }

        const position_t *IWrapper::position()
        {
            return NULL;
        }

        ICanvas *IWrapper::create_canvas(ICanvas *&cv, size_t width, size_t height)
        {
            return NULL;
        }

        core::KVTStorage *IWrapper::kvt_lock()
        {
            return NULL;
        }

        core::KVTStorage *IWrapper::kvt_trylock()
        {
            return NULL;
        }

        bool IWrapper::kvt_release()
        {
            return false;
        }

        void IWrapper::state_changed()
        {
        }

        void IWrapper::dump_plugin_state()
        {
            if (pPlugin == NULL)
                return;

            io::Path path;
            status_t res;
            if ((res = system::get_temporary_dir(&path)) != STATUS_OK)
            {
                lsp_warn("Could not obtain temporary directory: %d", int(res));
                return;
            }
//            if ((res = path.append_child(LSP_ARTIFACT_ID "-dumps")) != STATUS_OK) // TODO
            if ((res = path.append_child("dumps")) != STATUS_OK)
            {
                lsp_warn("Could not form path to directory: %d", int(res));
                return;
            }
            if ((res = path.mkdir(true)) != STATUS_OK)
            {
                lsp_warn("Could not create directory %s: %d", path.as_utf8(), int(res));
                return;
            }

            // Get current time
            system::localtime_t t;
            system::get_localtime(&t);

            const meta::plugin_t *meta = pPlugin->metadata();
            if (meta == NULL)
                return;

            // Build the file name
            LSPString fname;
            if (!fname.fmt_ascii("%04d%02d%02d-%02d%02d%02d-%03d-%s.json",
                    t.year, t.month, t.mday, t.hour, t.min, t.sec, int(t.nanos / 1000000),
                    meta->lv2_uid
                ))
            {
                lsp_warn("Could not format the file name");
                return;
            }

            if ((res = path.append_child(&fname)) != STATUS_OK)
            {
                lsp_warn("Could not form the file name: %d", int(res));
                return;
            }

            lsp_info("Dumping plugin state to file:\n%s...", path.as_utf8());

            core::JsonDumper v;
            if ((res = v.open(&path)) != STATUS_OK)
            {
                lsp_warn("Could not create file %s: %d", path.as_utf8(), int(res));
                return;
            }

            v.begin_raw_object();
            {
                LSPString tmp;

                v.write("name", meta->name);
                v.write("description", meta->description);
//                v.write("package", LSP_MAIN_VERSION); // TODO
                tmp.fmt_ascii("%d.%d.%d",
                        int(LSP_MODULE_VERSION_MAJOR(meta->version)),
                        int(LSP_MODULE_VERSION_MINOR(meta->version)),
                        int(LSP_MODULE_VERSION_MICRO(meta->version))
                    );
                v.write("version", tmp.get_utf8());
//                tmp.fmt_ascii("%s%s", LSP_URI(lv2), meta->lv2_uid); // TODO
                v.write("lv2_uri", tmp.get_utf8());
                v.write("vst_id", meta->vst_uid);
                v.write("ladspa_id", meta->ladspa_id);
                v.write("this", pPlugin);
                v.begin_raw_object("data");
                {
                    pPlugin->dump(&v);
                }
                v.end_raw_object();
            }

            v.end_raw_object();
            v.close();

            lsp_info("State has been dumped to file:\n%s", path.as_utf8());
        }
    }
}



