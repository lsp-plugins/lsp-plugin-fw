/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 дек. 2021 г.
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

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/util/common/checksum.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/io/Dir.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

namespace lsp
{
    namespace vst2_make
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

        status_t gen_cpp_file(const io::Path *file, const meta::plugin_t *meta)
        {
            LSPString name;
            status_t res;

            if ((res = file->get_last(&name)) != STATUS_OK)
                return res;

            // Generate file
            FILE *out = fopen(file->as_native(), "w");
            if (out == NULL)
            {
                int code = errno;
                fprintf(stderr, "Error creating file '%s', code=%d\n", file->as_native(), code);
                return STATUS_IO_ERROR;
            }

            // Write to file
            fprintf(out,    "//------------------------------------------------------------------------------\n");
            fprintf(out,    "// File:            %s\n", name.get_utf8());
            fprintf(out,    "// VST2 Plugin:     %s - %s [VST2]\n", meta->name, meta->description);
            fprintf(out,    "// VST2 UID:        '%s'\n", meta->vst2_uid);
            fprintf(out,    "// Version:         %d.%d.%d\n",
                    LSP_MODULE_VERSION_MAJOR(meta->version),
                    LSP_MODULE_VERSION_MINOR(meta->version),
                    LSP_MODULE_VERSION_MICRO(meta->version)
                );
            fprintf(out,    "//------------------------------------------------------------------------------\n\n");

            // Write code
            fprintf(out,    "// Pass Plugin UID for factory function\n");
            fprintf(out,    "#define VST2_PLUGIN_UID     \"%s\"\n", meta->vst2_uid);
            fprintf(out,    "\n");

            fprintf(out,    "#include <lsp-plug.in/common/types.h>\n");
            fprintf(out,    "\n");

            fprintf(out,    "// Include factory function implementation\n");
            fprintf(out,    "#define LSP_PLUG_IN_VST2_MAIN_IMPL\n");
            fprintf(out,    "    #include <lsp-plug.in/plug-fw/wrap/vst2/main.h>\n");
            fprintf(out,    "#undef LSP_PLUG_IN_VST2_MAIN_IMPL\n");

            // Close file
            fclose(out);

            return STATUS_OK;
        }

        static status_t gen_source_file(const io::Path *file, const meta::plugin_t *meta)
        {
            status_t res;

            // Generate temporary file
            if (file->exists())
            {
                io::Path temp;
                if ((res = temp.set(file)) != STATUS_OK)
                    return res;
                if ((res = temp.append(".tmp")) != STATUS_OK)
                    return res;

                // Generate temporary file
                if ((res = gen_cpp_file(&temp, meta)) != STATUS_OK)
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
                if ((res = gen_cpp_file(file, meta)) != STATUS_OK)
                    return res;

                // Output information
                printf("Generated source file '%s'\n", file->as_native());
            }

            return STATUS_OK;
        }

        status_t make_filename(LSPString *fname, const meta::plugin_t *meta)
        {
            if (!fname->set_ascii(meta->uid))
                return STATUS_NO_MEM;
            fname->replace_all('_', '-');
            return STATUS_OK;
        }

        status_t gen_makefile(const io::Path *base, lltl::parray<meta::plugin_t> *list)
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

            fprintf(out, "# Output files\n");
            fprintf(out, "OBJ_FILES = \\\n");
            for (size_t i=0, n=list->size(); i<n; )
            {
                // Get plugin metadata
                const meta::plugin_t *meta = list->uget(i);
                if ((res = make_filename(&fname, meta)) != STATUS_OK)
                    return res;

                fprintf(out, "  %s$(LIBRARY_EXT)", fname.get_utf8());
                if (++i >= n)
                    fprintf(out, "\n\n");
                else
                    fprintf(out, " \\\n");
            }

            fprintf(out, "FILE = $(@:%%$(LIBRARY_EXT)=%%.cpp)\n");
            fprintf(out, "DEP_CXX = $(foreach src,$(CXX_FILES),$(patsubst %%.cpp,%%.d,$(src)))\n");
            fprintf(out, "DEP_CXX_FILE = $(patsubst %%.d,%%.cpp,$(@))\n");
            fprintf(out, "DEP_DEP_FILE = $(patsubst %%.d,%%$(LIBRARY_EXT),$(@))\n");
            fprintf(out, "VST2_CXX_DEFS = -DEXT_ARTIFACT_ID=\\\"$(EXT_ARTIFACT_ID)\\\"\n");
            fprintf(out, "\n");

            fprintf(out, ".DEFAULT_GOAL := all\n");
            fprintf(out, ".PHONY: all install dep_clean depend\n");

            fprintf(out, "\n");
            fprintf(out, "$(DEP_CXX): dep_clean\n");
            fprintf(out, "\t$(CXX) -MM -MT \"$(DEP_DEP_FILE)\" -MF $(@) $(CXXFLAGS) $(EXT_CXXFLAGS) $(VST2_CXX_DEFS) $(INCLUDE) $(EXT_INCLUDE) $(DEP_CXX_FILE)\n");

            fprintf(out, "\n");
            fprintf(out, "depend: $(DEP_CXX)\n");
            fprintf(out, "\tcat $(DEP_CXX) >Makefile.d\n");

            fprintf(out, "\n");
            fprintf(out, "all: $(OBJ_FILES)\n");

            fprintf(out, "\n");
            fprintf(out, "$(OBJ_FILES):\n");
            fprintf(out, "\techo \"  $(CXX) $(FILE)\"\n");
            fprintf(out, "\t$(CXX) -o $(@) $(CXXFLAGS) $(EXT_CXXFLAGS) $(VST2_CXX_DEFS) $(INCLUDE) $(EXT_INCLUDE) $(FILE) $(EXT_OBJS) $(LIBS) $(SO_FLAGS) $(EXT_LDFLAGS)\n");

            fprintf(out, "\n");
            fprintf(out, "install: $(OBJ_FILES)\n");
            fprintf(out, "\t$(INSTALL) $(OBJ_FILES) $(DESTDIR)/\n");

            fprintf(out, "\n");
            fprintf(out, "# Dependencies\n");
            fprintf(out, "-include Makefile.d\n");

            // Close file
            fclose(out);

            return 0;
        }

        status_t generate_files(const char *out_dir)
        {
            io::Path path, file;
            LSPString fname;
            lltl::parray<meta::plugin_t> list;

            status_t res;

            // Initialize path
            if ((res = path.set(out_dir)) != STATUS_OK)
                return res;

            // Enumerate plugins
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
                if ((res = gen_source_file(&file, meta)) != STATUS_OK)
                    return res;
            }

            return gen_makefile(&path, &list);
        }

        int main(int argc, const char **argv)
        {
            if (argc < 2)
                fprintf(stderr, "required destination path");
            lsp::status_t res = generate_files(argv[1]);
            if (res != lsp::STATUS_OK)
                fprintf(stderr, "Error while generating build files, code=%d", int(res));

            return res;
        }
    } /* vst2_make */
} /* lsp */
