/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 февр. 2024 г.
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
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/modinfo.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

namespace lsp
{
    namespace vst3_modinfo
    {
        typedef struct cmdline_t
        {
            const char *modinfo;        // Path to moduleinfo.json
            const char *info_plist;     // Path to Info.plist
        } cmdline_t;

        static status_t parse_cmdline(cmdline_t *cfg, int argc, const char **argv)
        {
            cfg->modinfo        = NULL;
            cfg->info_plist     = NULL;

            // Parse arguments
            int i = 1;

            while (i < argc)
            {
                const char *arg = argv[i++];
                if ((!::strcmp(arg, "--help")) || (!::strcmp(arg, "-h")))
                {
                    printf("Usage: %s [parameters] [resource-directories]\n\n", argv[0]);
                    printf("Available parameters:\n");
                    printf("  -h, --help                    Show help\n");
                    printf("  -i, --info-plist <file>       Generate Info.plist file contents to the specified file\n");
                    printf("  -m, --modinfo <file>          Write modinfo.json file contents to the specified file\n");
                    printf("\n");

                    return STATUS_CANCELLED;
                }
                else if ((!::strcmp(arg, "--info-plist")) || (!::strcmp(arg, "-i")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified file name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->info_plist)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->info_plist = argv[i++];
                }
                else if ((!::strcmp(arg, "--modinfo")) || (!::strcmp(arg, "-m")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified file name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->modinfo)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->modinfo = argv[i++];
                }
                else
                {
                    fprintf(stderr, "Unknown argument '%s'\n", arg);
                    return STATUS_BAD_ARGUMENTS;
                }
            }

            return STATUS_OK;
        }

        static status_t write_modinfo(const char *file, const meta::package_t *manifest)
        {
            // Initialize path
            status_t res;
            io::Path path;
            if ((res = path.set_native(file)) != STATUS_OK)
            {
                fprintf(stderr, "Error parsing moduleinfo.json path, error=%d\n", int(res));
                return res;
            }

            // Make moduleinfo file
            if ((res = vst3::make_moduleinfo(&path, manifest)) != STATUS_OK)
            {
                fprintf(stderr, "Error creating moduleinfo.json file, error=%d\n", int(res));
                return res;
            }

            return STATUS_OK;
        }

        static status_t write_escaped_string(io::OutSequence *os, const LSPString *string)
        {
            LSPString out;

            if (!out.reserve(string->length()))
                return STATUS_NO_MEM;
            if (!out.append_ascii("\t<string>"))
                return STATUS_NO_MEM;

            // Escape characters
            for (size_t i=0, n=string->length(); i<n; ++i)
            {
                const lsp_wchar_t ch = string->char_at(i);
                if (ch < 0x20u)
                {
                    switch (ch)
                    {
                        case '\t':
                            if (!out.append(ch))
                                return STATUS_NO_MEM;
                            break;

                        default:
                            if (!out.fmt_append_ascii("&x%02x;", int(ch)))
                                return STATUS_NO_MEM;
                            break;
                    }
                }
                else if (ch < 0x80u)
                {
                    switch (ch)
                    {
                        case '&':
                            if (!out.append_ascii("&amp;"))
                                return STATUS_NO_MEM;
                            break;
                        case '<':
                            if (!out.append_ascii("&lt;"))
                                return STATUS_NO_MEM;
                            break;
                        case '>':
                            if (!out.append_ascii("&gt;"))
                                return STATUS_NO_MEM;
                            break;
                        case '\"':
                            if (!out.append_ascii("&quot;"))
                                return STATUS_NO_MEM;
                            break;
                        case '\'':
                            if (!out.append_ascii("&apos;"))
                                return STATUS_NO_MEM;
                            break;
                        default:
                            if (!out.append(ch))
                                return STATUS_NO_MEM;
                            break;
                    }
                }
                else
                {
                    if (!out.fmt_append_ascii("&x%04x;", int(ch)))
                        return STATUS_NO_MEM;
                }
            }

            // Output string
            if (!out.append_ascii("</string>"))
                return STATUS_NO_MEM;

            return os->writeln(&out);
        }

        static status_t write_escaped_string(io::OutSequence *os, const char *string)
        {
            LSPString tmp, out;
            if (!tmp.set_utf8(string))
                return STATUS_NO_MEM;

            return write_escaped_string(os, &tmp);
        }

        static inline status_t write_key(io::OutSequence *os, const char *key)
        {
            LSP_STATUS_ASSERT(os->write_ascii("\t<key>"));
            LSP_STATUS_ASSERT(os->write_ascii(key));
            LSP_STATUS_ASSERT(os->writeln_ascii("<key>"));
            return STATUS_OK;
        };

        static status_t write_info_plist(const char *file, const meta::package_t *manifest)
        {
            // Initialize path
            status_t res;
            io::Path path;
            if ((res = path.set_native(file)) != STATUS_OK)
            {
                fprintf(stderr, "Error parsing Info.plist path, error=%d\n", int(res));
                return res;
            }

            // Create output file
            io::OutSequence os;
            if ((res = os.open(&path, io::File::FM_WRITE_NEW, "UTF-8")) != STATUS_OK)
            {
                fprintf(stderr, "Error writing Info.plist path, error=%d\n", int(res));
                return res;
            }
            lsp_finally {
                os.close();
            };

            // Generate Info.plist contents
            LSP_STATUS_ASSERT(os.writeln_ascii("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
            LSP_STATUS_ASSERT(os.writeln_ascii("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"));
            LSP_STATUS_ASSERT(os.writeln_ascii("<plist version=\"1.0\">"));
            {
                // Generate version string
                LSPString version;
                if (version.fmt_ascii("%d.%d.%d",
                    manifest->version.major,
                    manifest->version.minor,
                    manifest->version.micro) < 0)
                    return STATUS_NO_MEM;

                LSP_STATUS_ASSERT(write_key(&os, "CFBundleName"));
                LSP_STATUS_ASSERT(write_escaped_string(&os, manifest->artifact));

                LSP_STATUS_ASSERT(write_key(&os, "CFBundleDisplayName"));
                LSP_STATUS_ASSERT(write_escaped_string(&os, manifest->artifact_name));

                LSP_STATUS_ASSERT(write_key(&os, "CFBundlePackageType"));
                LSP_STATUS_ASSERT(write_escaped_string(&os, "BNDL"));

                LSP_STATUS_ASSERT(write_key(&os, "CFBundleVersion"));
                LSP_STATUS_ASSERT(write_escaped_string(&os, &version));

                LSP_STATUS_ASSERT(write_key(&os, "CFBundleShortVersionString"));
                LSP_STATUS_ASSERT(write_escaped_string(&os, &version));

                LSP_STATUS_ASSERT(write_key(&os, "CFBundleSignature"));
                LSP_STATUS_ASSERT(write_escaped_string(&os, "????"));

                LSP_STATUS_ASSERT(write_key(&os, "CFBundleSupportedPlatforms"));
                LSP_STATUS_ASSERT(os.writeln_ascii("\t<array>"));
                    LSP_STATUS_ASSERT(os.writeln_ascii("\t\t<string>MacOSX</string>"));
                LSP_STATUS_ASSERT(os.writeln_ascii("\t</array>"));

                LSP_STATUS_ASSERT(write_key(&os, "NSHighResolutionCapable"));
                LSP_STATUS_ASSERT(os.writeln_ascii("\t<true/>"));

                LSP_STATUS_ASSERT(write_key(&os, "NSHumanReadableCopyright"));
                LSP_STATUS_ASSERT(write_escaped_string(&os, manifest->copyright));
            }
            LSP_STATUS_ASSERT(os.writeln_ascii("</plist>"));

            return STATUS_OK;
        }

        int main(int argc, const char **argv)
        {
            // Parse command line options
            cmdline_t cmd;
            status_t res = parse_cmdline(&cmd, argc, argv);
            if (res != STATUS_OK)
                return res;

            // Create resource loader
            resource::ILoader *loader   = core::create_resource_loader();
            if (loader == NULL)
            {
                fprintf(stderr, "No resource loader available\n");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                delete loader;
            };

            meta::package_t *manifest = NULL;
            {
                // Obtain the manifest file descriptor
                io::IInStream *is = loader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
                if (is == NULL)
                {
                    fprintf(stderr, "No manifest file found\n");
                    return STATUS_BAD_STATE;
                }
                lsp_finally {
                    is->close();
                    delete is;
                };

                // Read manifest data
                if ((res = meta::load_manifest(&manifest, is)) != STATUS_OK)
                {
                    fprintf(stderr, "Error loading manifest file, error=%d\n", int(res));
                    return res;
                }
            }
            if (manifest == NULL)
            {
                fprintf(stderr, "No manifest has been loaded\n");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                meta::free_manifest(manifest);
            };

            // Write the moduleinfo.json file
            if (cmd.modinfo != NULL)
            {
                if ((res = write_modinfo(cmd.modinfo, manifest)) != STATUS_OK)
                    return res;
            }

            // Write the Info.plist file
            if (cmd.info_plist != NULL)
            {
                if ((res = write_info_plist(cmd.info_plist, manifest)) != STATUS_OK)
                    return res;
            }

            // All seems to be OK
            return res;
        }
    } /* namespace vst3_modinfo */
} /* namespace lsp */


