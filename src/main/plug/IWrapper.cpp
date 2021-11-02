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
        IWrapper::IWrapper(Module *plugin, resource::ILoader *loader)
        {
            pPlugin         = plugin;
            pLoader         = loader;
            pCanvasFactory  = NULL;
        }

        IWrapper::~IWrapper()
        {
            pPlugin         = NULL;
            pLoader         = NULL;
            pCanvasFactory  = NULL;
        }

        ipc::IExecutor *IWrapper::executor()
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

        plug::ICanvasFactory *IWrapper::set_canvas_factory(plug::ICanvasFactory *factory)
        {
            plug::ICanvasFactory *old = pCanvasFactory;
            pCanvasFactory = factory;
            return old;
        }

        void IWrapper::dump_plugin_state()
        {
            if (pPlugin == NULL)
                return;

            const meta::package_t *pkg = package();

            LSPString tmp;
            io::Path path;
            status_t res;
            if ((res = system::get_temporary_dir(&path)) != STATUS_OK)
            {
                lsp_warn("Could not obtain temporary directory: %d", int(res));
                return;
            }

            if (tmp.fmt_utf8("%s-dumps", pkg->artifact) <= 0)
            {
                lsp_warn("Could not form path to directory: %d", int(res));
                return;
            }
            if ((res = path.append_child(&tmp)) != STATUS_OK)
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
                    meta->uid
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
                v.write("name", meta->name);
                v.write("description", meta->description);
                v.write("artifact", pkg->artifact);

                // Package version
                tmp.fmt_ascii("%d.%d.%d",
                        int(pkg->version.major),
                        int(pkg->version.minor),
                        int(pkg->version.micro)
                );
                if (pkg->version.branch)
                    tmp.fmt_append_utf8("-%s", pkg->version.branch);
                v.write("package", tmp.get_utf8());

                // Version
                tmp.fmt_ascii("%d.%d.%d",
                        int(LSP_MODULE_VERSION_MAJOR(meta->version)),
                        int(LSP_MODULE_VERSION_MINOR(meta->version)),
                        int(LSP_MODULE_VERSION_MICRO(meta->version))
                    );
                v.write("version", tmp.get_utf8());

                // Write plugin identifiers
                v.write("lv2_uri", tmp.get_utf8());
                v.write("vst_id", meta->vst_uid);
                v.write("ladspa_id", meta->ladspa_id);
                v.write("ladspa_label", meta->ladspa_lbl);

                // Dump object contents
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

        const meta::package_t *IWrapper::package() const
        {
            return NULL;
        }
    }
}



