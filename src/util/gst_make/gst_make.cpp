/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 25 мая 2024 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/plug.h>

#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/util/common/checksum.h>
#include <lsp-plug.in/plug-fw/util/gst_make/gst_make.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/stdlib.h>
#include <lsp-plug.in/io/InSequence.h>
#include <lsp-plug.in/io/OutStringSequence.h>

#include <errno.h>

namespace lsp
{
    namespace gst_make
    {
        static ssize_t meta_sort_func(const meta::plugin_t *a, const meta::plugin_t *b)
        {
            return strcmp(a->uid, b->uid);
        }

        status_t enumerate_plugins(lltl::parray<meta::plugin_t> *list)
        {
            // Iterate over all available factories
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Check that gstreamer is supported by plugin
                    if (meta->uids.gst == NULL)
                        continue;

                    // Add metadata to list
                    if (!list->add(const_cast<meta::plugin_t *>(meta)))
                    {
                        fprintf(stderr, "Error adding plugin to list: '%s'\n", meta->uid);
                        return STATUS_NO_MEM;
                    }
                }
            }

            // Sort the metadata
            list->qsort(meta_sort_func);

            return STATUS_OK;
        }

        status_t make_filename(LSPString *fname, const meta::plugin_t *meta)
        {
            if (!fname->set_ascii("libgst"))
                return STATUS_NO_MEM;
            if (!fname->append_ascii(meta->uids.gst))
                return STATUS_NO_MEM;

            fname->replace_all('_', '-');
            fname->tolower();

            return STATUS_OK;
        }

        status_t read_template_file(LSPString *dst, const char *file)
        {
            status_t res;

            // Open input sequence
            io::InSequence is;
            if ((res = is.open(file)) != STATUS_OK)
                return res;
            lsp_finally {
                is.close();
            };
            lsp_info("Open OK");

            // Open output sequence
            LSPString tmp;
            io::OutStringSequence os;
            if ((res = os.wrap(&tmp, false)) != STATUS_OK)
                return res;
            lsp_finally {
                os.close();
            };
            lsp_info("Wrap OK");

            // Copy contents to the sink
            wssize_t read = is.sink(&os);
            if (read < 0)
                return status_t(-read);
            lsp_info("Sink OK");

            // Commit result and return OK status
            tmp.swap(dst);
            return STATUS_OK;
        }

        const char *get_license(const char *license)
        {
            if ((license == NULL) || (strlen(license) == 0))
                return "unknown";
            if (strcasestr(license, "lgpl") != NULL)
                return "LGPL";
            if (strcasestr(license, "qpl") != NULL)
            {
                if (strcasestr(license, "gpl") != NULL)
                    return "GPL/QPL";
                return "QPL";
            }
            if (strcasestr(license, "gpl") != NULL)
                return "GPL";
            if (strcasestr(license, "mpl") != NULL)
                return "MPL";
            if (strcasestr(license, "bsd") != NULL)
                return "BSD";
            if ((strcasestr(license, "mit") != NULL) || (strcasestr(license, "x11") != NULL))
                return "MID/X11";

            return "Proprietary";
        }

        bool make_camel_case(LSPString *dst, const LSPString *src)
        {
            LSPString tmp;
            const size_t length = src->length();
            if (!tmp.reserve(length))
                return false;

            bool upper = true;
            for (size_t i=0; i<length; ++i)
            {
                lsp_wchar_t c = src->char_at(i);
                if ((c == '_') || (c == '-'))
                {
                    upper = true;
                    continue;
                }

                if (upper)
                {
                    upper = false;
                    if ((c >= 'a') && (c <= 'z'))
                        c   = c - 'a' + 'A';
                }
                else if ((c >= 'A') && (c <= 'Z'))
                    c   = c - 'A' + 'a';

                if (!tmp.append(c))
                    return false;
            }

            tmp.swap(dst);

            return true;
        }

        status_t gen_cpp_file(const io::Path *file, const char *fname, const LSPString *ftemplate, const meta::package_t *package, const meta::plugin_t *meta)
        {
            // Generate file
            FILE *out = fopen(file->as_native(), "w");
            if (out == NULL)
            {
                int code = errno;
                fprintf(stderr, "Error creating file '%s', code=%d\n", file->as_native(), code);
                return STATUS_IO_ERROR;
            }
            lsp_finally {
                fclose(out);
            };

            // Write to file
            fprintf(out,    "//------------------------------------------------------------------------------\n");
            fprintf(out,    "// Automatically generated file, do not edit\n");
            fprintf(out,    "// \n");
            if (fname != NULL)
                fprintf(out,    "// File:            %s\n", fname);
            fprintf(out,    "// GST Plugin:      %s - %s [GStreamer]\n", meta->name, meta->description);
            fprintf(out,    "// GST UID:         '%s'\n", meta->uids.gst);
            fprintf(out,    "// Version:         %d.%d.%d\n",
                    LSP_MODULE_VERSION_MAJOR(meta->version),
                    LSP_MODULE_VERSION_MINOR(meta->version),
                    LSP_MODULE_VERSION_MICRO(meta->version)
                );
            fprintf(out,    "//------------------------------------------------------------------------------\n\n");

            // Pass plugin metadata definitions
            LSPString package_id;
            if (!package_id.set_utf8(package->brand_id))
                return STATUS_NO_MEM;
            if (!package_id.append('-'))
                return STATUS_NO_MEM;
            if (!package_id.append_utf8(meta->bundle->uid))
                return STATUS_NO_MEM;
            package_id.replace_all('_', '-');

            fprintf(out,    "// Pass Plugin metadata definitions\n");
            fprintf(out,    "#define DEF_PLUGIN_PACKAGE_ID           \"%s\"\n", package_id.get_utf8());
            fprintf(out,    "#define DEF_PLUGIN_PACKAGE_NAME         \"%s %s\"\n", package->brand, meta->bundle->name);
            fprintf(out,    "#define DEF_PLUGIN_ID                   \"%s\"\n", meta->uids.gst);
            fprintf(out,    "#define DEF_PLUGIN_NAME                 \"%s\"\n", meta->description);
            fprintf(out,    "#define DEF_PLUGIN_VERSION_STR          \"%d.%d.%d\"\n", int(meta->version.major), int(meta->version.minor), int(meta->version.micro));
            fprintf(out,    "#define DEF_PLUGIN_LICENSE              \"%s\"\n", get_license(package->license));
            fprintf(out,    "#define DEF_PLUGIN_ORIGIN               \"%s\"\n", package->site);
            fprintf(out,    "\n\n");

            // Preprocess template file
            LSPString plugin_upper, plugin_lower, plugin_camel, plugin_canonical;

            if (!plugin_upper.set_ascii(meta->uids.gst))
                return STATUS_NO_MEM;
            plugin_upper.toupper();

            if (!plugin_lower.set(&plugin_upper))
                return STATUS_NO_MEM;
            plugin_lower.tolower();

            if (!make_camel_case(&plugin_camel, &plugin_lower))
                return STATUS_NO_MEM;

            if (!plugin_canonical.set(&plugin_lower))
                return STATUS_NO_MEM;
            plugin_canonical.replace_all('_', '-');

            ssize_t pos = 0;
            const ssize_t length = ftemplate->length();
            while (pos < length)
            {
                // Find the first 'x' or 'X' occurrence
                ssize_t index  = ftemplate->index_of(pos, 'x');
                ssize_t index2 = ftemplate->index_of(pos, 'X');
                if (index < 0)
                {
                    index       = index2;
                    if (index < 0)
                        break;
                }
                else if (index2 >= 0)
                    index       = lsp_min(index, index2);

                // Output not affected text
                if (pos < index)
                    fputs(ftemplate->get_utf8(pos, index), out);

                // Check pattern
                if (ftemplate->starts_with_ascii("xx_plugin_id_xx", index))
                {
                    fputs(plugin_lower.get_utf8(), out);
                    pos    = index + strlen("xx_plugin_id_xx");
                }
                else if (ftemplate->starts_with_ascii("XX_PLUGIN_ID_XX", index))
                {
                    fputs(plugin_upper.get_utf8(), out);
                    pos    = index + strlen("XX_PLUGIN_ID_XX");
                }
                else if (ftemplate->starts_with_ascii("Xx_PluginId_Xx", index))
                {
                    fputs(plugin_camel.get_utf8(), out);
                    pos    = index + strlen("Xx_PluginId_Xx");
                }
                else if (ftemplate->starts_with_ascii("xc_plugin_id_cx", index))
                {
                    fputs(plugin_canonical.get_utf8(), out);
                    pos    = index + strlen("xc_plugin_id_cx");
                }
                else // didn't match
                {
                    fputs(ftemplate->get_utf8(index, index+1), out);
                    pos     = index + 1;
                }
            }

            // Write the rest contents
            if (pos < length)
                fputs(ftemplate->get_utf8(pos), out);

            return STATUS_OK;
        }

        status_t gen_source_file(const io::Path *file, const LSPString *ftemplate, const meta::package_t *package, const meta::plugin_t *meta)
        {
            LSPString fname;
            io::Path temp;
            status_t res;

            // Get file name
            if ((res = file->get_last(&fname)) != STATUS_OK)
                return STATUS_NO_MEM;

            // Generate temporary file
            if (file->exists())
            {
                // Generate temporary file name
                if ((res = temp.set(file)) != STATUS_OK)
                    return res;
                if ((res = temp.append(".tmp")) != STATUS_OK)
                    return res;

                // Generate temporary file
                if ((res = gen_cpp_file(&temp, fname.get_native(), ftemplate, package, meta)) != STATUS_OK)
                    return res;

                // Compute checksums of file
                util::checksum_t f1, f2;
                if ((res = util::calc_checksum(&f1, &temp)) != STATUS_OK)
                    return res;
                if ((res = util::calc_checksum(&f2, file)) != STATUS_OK)
                    return res;

                if (!util::match_checksum(&f1, &f2))
                {
                    if ((res = temp.rename(file)) != STATUS_OK)
                    {
                        fprintf(stderr, "Could not rename file '%s' to '%s'\n", temp.as_native(), file->as_native());
                        return res;
                    }

                    // Output information
                    printf("Generated source file '%s'\n", file->as_native());
                }
                else
                {
                    if ((res = temp.remove()) != STATUS_OK)
                    {
                        fprintf(stderr, "Could not remove file: '%s'\n", file->as_native());
                        return res;
                    }
                }
            }
            else
            {
                // Generate direct file
                if ((res = gen_cpp_file(file, fname.get_native(), ftemplate, package, meta)) != STATUS_OK)
                    return res;

                // Output information
                printf("Generated source file '%s'\n", file->as_native());
            }

            return STATUS_OK;
        }

        status_t gen_makefile(const io::Path *base, lltl::parray<meta::plugin_t> *list, const meta::package_t *package)
        {
            io::Path path;
            LSPString fname;
            status_t res;

            if ((res = path.set(base, "Makefile")) != STATUS_OK)
                return res;
            printf("Generating makefile\n");

            // Generate file
            FILE *out = fopen(path.as_native(), "w");
            if (out == NULL)
            {
                int code = errno;
                fprintf(stderr, "Error creating file %s, errno=%d\n", path.as_native(), code);
                return STATUS_IO_ERROR;
            }
            lsp_finally { fclose(out); };

            fprintf(out, "# Automatically generated makefile, do not edit\n");
            fprintf(out, "\n");

            fprintf(out, "# Command-line flag to silence nested $(MAKE).\n");
            fprintf(out, "ifneq ($(VERBOSE),1)\n");
            fprintf(out, ".SILENT:\n");
            fprintf(out, "endif\n");
            fprintf(out, "\n");

            fprintf(out, "CONFIG := $(CURDIR)/.config.mk\n");
            fprintf(out, "SETTINGS := $(CURDIR)/.settings.mk\n\n");
            fprintf(out, "include $(CONFIG)\n");
            fprintf(out, "include $(SETTINGS)\n\n");

            fprintf(out, "# Source files\n");
            fprintf(out, "CXX_FILES = \\\n");
            for (size_t i=0, n=list->size(); i<n; )
            {
                // Get plugin metadata
                const meta::plugin_t *meta = list->uget(i);
                if ((res = make_filename(&fname, meta)) != STATUS_OK)
                    return res;

                fprintf(out, "  %s.cpp", fname.get_utf8());
                if (++i >= n)
                    fprintf(out, "\n\n");
                else
                    fprintf(out, " \\\n");
            }
            fprintf(out, "\n");

            fprintf(out, "# Output files\n");
            fprintf(out, "OBJ_FILES = \\\n");
            for (size_t i=0, n=list->size(); i<n; )
            {
                // Get plugin metadata
                const meta::plugin_t *meta = list->uget(i);
                if ((res = make_filename(&fname, meta)) != STATUS_OK)
                    return res;

                fprintf(out, "  $(EXT_PREFIX)%s$(LIBRARY_EXT)", fname.get_utf8());
                if (++i >= n)
                    fprintf(out, "\n\n");
                else
                    fprintf(out, " \\\n");
            }
            fprintf(out, "\n");

            fprintf(out, "FILE = $(@:$(EXT_PREFIX)%%$(LIBRARY_EXT)=%%.cpp)\n");
            fprintf(out, "DEP_CXX = $(foreach src,$(CXX_FILES),$(patsubst %%.cpp,%%.d,$(src)))\n");
            fprintf(out, "DEP_CXX_FILE = $(patsubst %%.d,%%.cpp,$(@))\n");
            fprintf(out, "DEP_DEP_FILE = $(patsubst %%.d,$(EXT_PREFIX)%%$(LIBRARY_EXT),$(@))\n");
            fprintf(out, "\n");

            fprintf(out, ".DEFAULT_GOAL := all\n");
            fprintf(out, ".PHONY: all install dep_clean depend\n");

            fprintf(out, "\n");
            fprintf(out, "$(DEP_CXX): dep_clean\n");
            fprintf(out, "\t$(CXX) -MM -MT \"$(DEP_DEP_FILE)\" -MF $(@) $(CXXFLAGS) $(CXXDEFS) $(EXT_CXXFLAGS) $(INCLUDE) $(EXT_INCLUDE) $(DEP_CXX_FILE)\n");

            fprintf(out, "\n");
            fprintf(out, "depend: $(DEP_CXX)\n");
            fprintf(out, "\tcat $(DEP_CXX) >Makefile.d\n");

            fprintf(out, "\n");
            fprintf(out, "all: $(OBJ_FILES)\n");

            fprintf(out, "\n");
            fprintf(out, "$(OBJ_FILES):\n");
            fprintf(out, "\techo \"  $(CXX)  [gst] $(FILE)\"\n");
            fprintf(out, "\t$(CXX) -o $(@) $(CXXFLAGS) $(CXXDEFS) $(EXT_CXXFLAGS) $(INCLUDE) $(EXT_INCLUDE) $(FILE) $(EXT_OBJS) $(LIBS) $(SO_FLAGS) $(EXT_LDFLAGS)\n");

            fprintf(out, "\n");
            fprintf(out, "install: $(OBJ_FILES)\n");
            fprintf(out, "\t$(INSTALL) $(OBJ_FILES) $(DESTDIR)/\n");

            fprintf(out, "\n");
            fprintf(out, "uninstall: \n");
            fprintf(out, "\t-rm -f $(addprefix $(DESTDIR)/,$(OBJ_FILES))\n");

            fprintf(out, "\n");
            fprintf(out, "# Dependencies\n");
            fprintf(out, "-include Makefile.d\n");

            return 0;
        }

        status_t generate_files(const cmdline_t *cmd)
        {
            io::Path path, file;
            LSPString fname, ftemplate;
            lltl::parray<meta::plugin_t> list;

            status_t res;

            // Initialize path
            if ((res = path.set(cmd->destination)) != STATUS_OK)
                return res;

            // Read template file
            lsp_info("read_tempate_file %s", cmd->template_file);
            if ((res = read_template_file(&ftemplate, cmd->template_file)) != STATUS_OK)
            {
                fprintf(stderr, "Error reading template file '%s', error=%d\n", cmd->template_file, int(res));
                return res;
            }

            // Read manifest
            meta::package_t *package = NULL;
            lsp_info("load_manifest %s", cmd->manifest);
            if ((res = meta::load_manifest(&package, cmd->manifest)) != STATUS_OK)
            {
                fprintf(stderr, "Error loading manifest '%s', error=%d\n", cmd->manifest, int(res));
                return res;
            }
            if (package == NULL)
            {
                fprintf(stderr, "Error loading manifest '%s': empty manifest\n", cmd->manifest);
                return STATUS_UNKNOWN_ERR;
            }
            lsp_finally {
                meta::free_manifest(package);
            };

            // Enumerate plugins
            lsp_info("enumerate_plugins");
            if ((res = enumerate_plugins(&list)) != STATUS_OK)
                return res;

            printf("Enumerated %d plugins\n", int(list.size()));

            for (size_t i=0, n=list.size(); i<n; ++i)
            {
                // Get plugin metadata
                const meta::plugin_t *meta = list.uget(i);

                // Get file path
                if ((res = make_filename(&fname, meta)) != STATUS_OK)
                    return res;
                if (!fname.append_ascii(".cpp"))
                    return STATUS_NO_MEM;
                if ((res = file.set(&path, &fname)) != STATUS_OK)
                    return res;

                // Generate C++ file
                if ((res = gen_source_file(&file, &ftemplate, package, meta)) != STATUS_OK)
                {
                    fprintf(stderr, "Error generating source file %s: error code %d", file.as_native(), res);
                    return res;
                }
            }

            return gen_makefile(&path, &list, package);
        }

        status_t parse_cmdline(cmdline_t *cfg, int argc, const char **argv)
        {
            cfg->manifest       = NULL;
            cfg->template_file  = NULL;
            cfg->destination    = NULL;

            // Parse arguments
            int i = 1;

            while (i < argc)
            {
                const char *arg = argv[i++];
                if ((!::strcmp(arg, "--help")) || (!::strcmp(arg, "-h")))
                {
                    printf("Usage: %s [parameters]\n\n", argv[0]);
                    printf("Available parameters:\n");
                    printf("  -h, --help                    Show help\n");
                    printf("  -t, --template <dir>          The template file for preprocessing\n");
                    printf("  -m, --manifest <file>         Manifest file\n");
                    printf("  -o, --output <dir>            The name of the destination directory to place files\n");
                    printf("                                place resources\n");
                    printf("\n");

                    return STATUS_CANCELLED;
                }
                else if ((!::strcmp(arg, "--output")) || (!::strcmp(arg, "-o")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified directory name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->destination)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->destination = argv[i++];
                }
                else if ((!::strcmp(arg, "--manifest")) || (!::strcmp(arg, "-m")))
                {
                    if (cfg->manifest)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->manifest = argv[i++];
                }
                else if ((!::strcmp(arg, "--template")) || (!::strcmp(arg, "-t")))
                {
                    if (cfg->template_file)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->template_file = argv[i++];
                }
                else
                {
                    fprintf(stderr, "Unknown parameter '%s'\n", arg);
                    return STATUS_BAD_ARGUMENTS;
                }
            }

            // Validate mandatory arguments
            if (cfg->template_file == NULL)
            {
                fprintf(stderr, "Template file name is required\n");
                return STATUS_BAD_ARGUMENTS;
            }
            if (cfg->manifest == NULL)
            {
                fprintf(stderr, "Manifest file name is required\n");
                return STATUS_BAD_ARGUMENTS;
            }
            if (cfg->destination == NULL)
            {
                fprintf(stderr, "Output directory name is required\n");
                return STATUS_BAD_ARGUMENTS;
            }

            return STATUS_OK;
        }

        status_t main(int argc, const char **argv)
        {
            // Parse command-line arguments
            cmdline_t cmd;
            status_t res;

            if ((res = parse_cmdline(&cmd, argc, argv)) != STATUS_OK)
                return res;

            if ((res = generate_files(&cmd)) != lsp::STATUS_OK)
                fprintf(stderr, "Error while generating build files, code=%d", int(res));

            return res;
        }
    } /* namespace gst_make */
} /* namespace lsp */


