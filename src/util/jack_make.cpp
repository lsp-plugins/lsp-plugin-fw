/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 23 дек. 2020 г.
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
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/io/Dir.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

namespace lsp
{
    namespace jack_make
    {
        typedef struct checksum_t
        {
            uint64_t ck1;
            uint64_t ck2;
            uint64_t ck3;
        } checksum_t;

        static ssize_t meta_sort_func(const meta::plugin_t *a, const meta::plugin_t *b)
        {
            return strcmp(a->lv2_uid, b->lv2_uid);
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
                        fprintf(stderr, "Error adding plugin to list: '%s'\n", meta->lv2_uid);
                        return STATUS_NO_MEM;
                    }
                }
            }

            // Sort the metadata
            list->qsort(meta_sort_func);

            return STATUS_OK;
        }

        static bool match_checksum(const checksum_t *c1, const checksum_t *c2)
        {
            return (c1->ck1 == c2->ck1) &&
                   (c1->ck2 == c2->ck2) &&
                   (c1->ck3 == c2->ck3);
        }

        static status_t calc_checksum(checksum_t *dst, const io::Path *file)
        {
            uint8_t buf[0x400];

            dst->ck1   = 0;
            dst->ck2   = 0;
            dst->ck3   = 0xffffffff;

            FILE *in = fopen(file->as_native(), "r");
            if (in == NULL)
                return STATUS_OK;

            while (true)
            {
                int nread = fread(buf, sizeof(uint8_t), sizeof(buf), in);
                if (nread <= 0)
                    break;

                for (int i=0; i<nread; ++i)
                {
                    // Compute checksums
                    dst->ck1 = dst->ck1 + buf[i];
                    dst->ck2 = ((dst->ck2 << 9) | (dst->ck2 >> 55)) ^ buf[i];
                    dst->ck3 = ((dst->ck3 << 17) | (dst->ck3 >> 47)) ^ uint16_t(0xa5ff - buf[i]);
                }
            }

            fclose(in);

            return STATUS_OK;
        }

        static status_t gen_cpp_file(const io::Path *file, const LSPString *name, const meta::plugin_t *meta)
        {
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
            fprintf(out,    "// File:            %s\n", name->get_utf8());
            fprintf(out,    "// JACK Plugin:     %s - %s [JACK]\n", meta->name, meta->description);
            fprintf(out,    "// JACK UID:        '%s'\n", meta->lv2_uid);
            fprintf(out,    "// Version:         %d.%d.%d\n",
                    LSP_MODULE_VERSION_MAJOR(meta->version),
                    LSP_MODULE_VERSION_MINOR(meta->version),
                    LSP_MODULE_VERSION_MICRO(meta->version)
                );
            fprintf(out,    "//------------------------------------------------------------------------------\n\n");

            // Write code
            fprintf(out,    "// Pass Plugin UID for factory function\n");
            fprintf(out,    "#define JACK_PLUGIN_UID     \"%s\"\n", meta->lv2_uid);
            fprintf(out,    "\n");

            fprintf(out,    "#include <lsp-plug.in/common/types.h>\n");
            fprintf(out,    "\n");

            fprintf(out,    "static const ::lsp::version_t REQUIRED_PACKAGE_VERSION = LSP_DEF_VERSION(1, 0, 0);\n");
            fprintf(out,    "\n");

            fprintf(out,    "// Include factory function implementation\n");
            fprintf(out,    "#define LSP_PLUG_IN_JACK_MAIN_IMPL\n");
            fprintf(out,    "    #include <lsp-plug.in/plug-fw/wrap/jack/main.h>\n");
            fprintf(out,    "#undef LSP_PLUG_IN_JACK_MAIN_IMPL\n");

            // Close file
            fclose(out);

            return STATUS_OK;
        }

        static status_t gen_source_file(const io::Path *file, const io::Path *temp, const meta::plugin_t *meta)
        {
            LSPString fname;
            status_t res;

            // Get file name
            if ((res = file->get_last(&fname)) != STATUS_OK)
                return STATUS_NO_MEM;

            // Generate temporary file
            if (file->exists())
            {
                // Generate temporary file
                if ((res = gen_cpp_file(temp, &fname, meta)) != STATUS_OK)
                    return res;

                // Compute checksums of file
                checksum_t f1, f2;
                if ((res = calc_checksum(&f1, temp)) != STATUS_OK)
                    return res;
                if ((res = calc_checksum(&f2, file)) != STATUS_OK)
                    return res;

                if (!match_checksum(&f1, &f2))
                {
                    if ((res = temp->rename(file)) != STATUS_OK)
                    {
                        fprintf(stderr, "Could not rename file '%s' to '%s'\n", temp->as_native(), file->as_native());
                        return res;
                    }

                    // Output information
                    printf("Generated source file '%s'\n", file->as_native());
                }
                else
                {
                    if ((res = temp->remove()) != STATUS_OK)
                    {
                        fprintf(stderr, "Could not remove file: '%s'\n", file->as_native());
                        return res;
                    }
                }
            }
            else
            {
                // Generate direct file
                if ((res = gen_cpp_file(file, &fname, meta)) != STATUS_OK)
                    return res;

                // Output information
                printf("Generated source file '%s'\n", file->as_native());
            }

            return STATUS_OK;
        }

        status_t make_filename(LSPString *fname, const char *lv2_uid)
        {
            if (!fname->set_ascii(lv2_uid))
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

            fprintf(out, "# Auto generated makefile, do not edit\n\n");
            fprintf(out, "CONFIG := $(CURDIR)/.config.mk\n\n");
            fprintf(out, "include $(CONFIG)\n\n");

            fprintf(out, "# Output files\n");
            fprintf(out, "FILES = \\\n");
            for (size_t i=0, n=list->size(); i<n; )
            {
                // Get plugin metadata
                const meta::plugin_t *meta = list->uget(i);
                if ((res = make_filename(&fname, meta->lv2_uid)) != STATUS_OK)
                    return res;

                fprintf(out, "  %s$(EXECUTABLE_EXT)", fname.get_utf8());
                if (++i >= n)
                    fprintf(out, "\n\n");
                else
                    fprintf(out, " \\\n");
            }

            fprintf(out, "FILE = $(@:%%$(EXECUTABLE_EXT)=%%.cpp)\n");
            fprintf(out, "\n");

            fprintf(out, ".DEFAULT_GOAL := all\n");
            fprintf(out, ".PHONY: all install\n");

            fprintf(out, "\n");
            fprintf(out, "all: $(FILES)\n");

            fprintf(out, "\n");
            fprintf(out, "$(FILES):\n");
            fprintf(out, "\t@echo \"  $(CXX) $(FILE)\"\n");
            fprintf(out, "\t@echo $(CXX) -o $(@) $(CXXFLAGS) $(INCLUDE) $(FILE) $(LIBS) $(EXE_FLAGS) $(LDFLAGS)\n");
            fprintf(out, "\t@$(CXX) -o $(@) $(CXXFLAGS) $(INCLUDE) $(FILE) $(LIBS) $(EXE_FLAGS) $(LDFLAGS)\n");

            fprintf(out, "\n");
            fprintf(out, "install: $(FILES)\n");
            fprintf(out, "\t@$(INSTALL) $(FILES) $(TARGET_PATH)/\n");

            fprintf(out, "\n");
            fprintf(out, "# Dependencies\n");
            for (size_t i=0, n=list->size(); i<n; ++i)
            {
                // Get plugin metadata
                const meta::plugin_t *meta = list->uget(i);
                if ((res = make_filename(&fname, meta->lv2_uid)) != STATUS_OK)
                    return res;

                const char *name = fname.get_utf8();
                fprintf(out, "%s$(EXECUTABLE_EXT): %s.cpp\n", name, name);
            }

            // Close file
            fclose(out);

            return 0;
        }

        status_t main(const char *base)
        {
            io::Path path, file, temp;
            LSPString fname;
            lltl::parray<meta::plugin_t> list;

            status_t res;

            // Initialize path
            if ((res = path.set(base)) != STATUS_OK)
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
                if ((res = make_filename(&fname, meta->lv2_uid)) != STATUS_OK)
                    return res;
                if (!fname.append_ascii(".cpp"))
                    return STATUS_NO_MEM;
                if ((res = file.set(&path, &fname)) != STATUS_OK)
                    return res;
                if (!fname.append_ascii(".tmp"))
                    return STATUS_NO_MEM;
                if ((res = temp.set(&path, &fname)) != STATUS_OK)
                    return res;

                // Generate C++ file
                if ((res = gen_source_file(&file, &temp, meta)) != STATUS_OK)
                    return res;
            }

            return gen_makefile(&path, &list);
        }
    }
}

#ifndef LSP_IDE_DEBUG
int main(int argc, const char **argv)
{
    if (argc <= 0)
        fprintf(stderr, "required destination path");
    lsp::status_t res = lsp::jack_make::main(argv[1]);
    if (res != lsp::STATUS_OK)
        fprintf(stderr, "Error while generating build files, code=%d", int(res));

    return res;
}
#endif /* LSP_IDE_DEBUG */



