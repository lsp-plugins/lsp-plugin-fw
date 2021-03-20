/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 мар. 2021 г.
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
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/io/Dir.h>
#include <lsp-plug.in/io/IOutStream.h>
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/resource/Compressor.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>

namespace lsp
{
    namespace resource
    {
        class OutFileStream: public io::IOutStream
        {
            protected:
                FILE       *pOut;
                size_t      nTotal;

            public:
                explicit OutFileStream(FILE *fd)
                {
                    pOut        = fd;
                    nTotal      = 0;
                }

                virtual ~OutFileStream()
                {
                    pOut        = NULL;
                    nTotal      = 0;
                }

            protected:
                void    out_byte(uint8_t b)
                {
                    if (nTotal > 0)
                        fputs(", ", pOut);
                    if (!(nTotal & 0xf))
                        fputs("\n\t\t", pOut);

                    fprintf(pOut, "0x%02x", b);
                    ++nTotal;
                }

            public:
                virtual ssize_t write(const void *buf, size_t count)
                {
                    const uint8_t *p = reinterpret_cast<const uint8_t *>(buf);

                    for (size_t i=0; i<count; ++i)
                        out_byte(p[i]);

                    return count;
                }

                inline size_t   total() const { return nTotal; }
        };

        class state_t
        {
            public:
                lltl::pphash<LSPString, lltl::parray<io::Path> > ext;   // Files grouped by extension
                resource::Compressor    c;                              // Resource compressor
                FILE                   *fd;                             // File descriptor
                OutFileStream          *os;                             // Output stream
                wssize_t                in_bytes;                       // Overall size of input data

            public:
                explicit inline state_t()
                {
                    fd          = NULL;
                    os          = NULL;
                    in_bytes    = 0;
                }

                ~state_t()
                {
                }
        };

        void drop_context(state_t *ctx)
        {
            lltl::parray<lltl::parray<io::Path> > vp;
            ctx->ext.values(&vp);

            // Destroy compressor
            ctx->c.close();
            if (ctx->os != NULL)
            {
                ctx->os->close();
                delete ctx->os;
                ctx->os     = NULL;
            }

            // Close file descriptor
            if (ctx->fd != NULL)
            {
                fclose(ctx->fd);
                ctx->fd     = NULL;
            }

            // Destroy items
            for (size_t i=0, n=vp.size(); i<n; ++i)
            {
                lltl::parray<io::Path> *pa = vp.uget(i);
                if (pa == NULL)
                    continue;

                for (size_t j=0, m=pa->size(); j<m; ++j)
                {
                    io::Path *p = pa->uget(j);
                    if (p != NULL)
                        delete p;
                }

                pa->flush();
                delete pa;
            }

            vp.flush();
            ctx->ext.flush();

            // Drop context object
            delete ctx;
        }

        status_t add_file(state_t *ctx, const io::Path *file)
        {
            LSPString ext;
            status_t res;

            // Get extension
            if ((res = file->get_ext(&ext)) != STATUS_OK)
                return res;
            ext.tolower();

            // Create the list of files
            lltl::parray<io::Path> *pa = ctx->ext.get(&ext);
            if (pa == NULL)
            {
                pa  = new lltl::parray<io::Path>();
                if (pa == NULL)
                    return STATUS_NO_MEM;
                if (!ctx->ext.create(&ext, pa))
                    return STATUS_NO_MEM;
            }

            // Add the path to list
            io::Path *item = file->clone();
            if (item == NULL)
                return STATUS_NO_MEM;

            if (pa->add(item))
                return STATUS_OK;

            delete item;
            return STATUS_NO_MEM;
        }

        status_t scan_directory(state_t *ctx, const io::Path *base, const io::Path *p)
        {
            io::Dir d;
            io::Path child;
            LSPString item;
            status_t res;

            if ((res = d.open(p)) != STATUS_OK)
                return res;

            while ((res = d.read(&item)) == STATUS_OK)
            {
                // Skip dots
                if (io::Path::is_dots(&item))
                    continue;

                // Set path
                if ((res = child.set(p, &item)) != STATUS_OK)
                    return res;

                if (child.is_dir())
                {
                    if ((res = scan_directory(ctx, base, &child)) != STATUS_OK)
                        return res;
                }
                else
                {
                    if ((res = child.remove_base(base)) != STATUS_OK)
                        return res;

                    printf("  found file: %s\n", child.as_native());
                    if ((res = add_file(ctx, &child)) != STATUS_OK)
                        return res;
                }
            }

            // Close directory
            if (res != STATUS_EOF)
                d.close();
            else
                res = d.close();

            return res;
        }

        status_t create_resource_file(state_t *ctx, const char *dst)
        {
            FILE *fd;

            // Open file
            if ((fd = fopen(dst, "w+")) == NULL)
            {
                fprintf(stderr, "  Could not open file: %s\n", dst);
                return STATUS_IO_ERROR;
            }

            // Wrap file descriptor
            OutFileStream *os = new OutFileStream(fd);
            if (os == NULL)
                return STATUS_NO_MEM;

            // Save stream to context
            ctx->os     = os;
            ctx->fd     = fd;

            // Write some data
            fprintf(fd, "/*\n");
            fprintf(fd, " * Resource file, automatically generated, do not edit\n");
            fprintf(fd, " */\n\n");

            fprintf(fd, "#include <lsp-plug.in/common/types.h>\n");
            fprintf(fd, "#include <lsp-plug.in/resource/types.h>\n");
            fprintf(fd, "#include <lsp-plug.in/plug-fw/core/Resources.h>\n");
            fprintf(fd, "\n");

            // Anonimous namespace start
            fprintf(fd, "namespace {\n\n");
            fprintf(fd, "\tstatic const uint8_t data[] =\n");
            fprintf(fd, "\t{");

            // Initialize compressor
            return ctx->c.init(LSP_RESOURCE_BUFSZ, ctx->os);
        }

        status_t compress_data(state_t *ctx, const io::Path *base)
        {
            io::Path path;
            io::InFileStream ifs;
            status_t res;
            wssize_t bytes;
            lltl::parray<LSPString> vk;
            if (!ctx->ext.keys(&vk))
                return STATUS_NO_MEM;
            vk.qsort();

            for (size_t i=0, n=vk.size(); i<n; ++i)
            {
                // Fetch the sorted list of files
                const LSPString *ext = vk.uget(i);
                if (ext == NULL)
                    return STATUS_CORRUPTED;

                lltl::parray<io::Path> *vp = ctx->ext.get(ext);
                if (vp == NULL)
                    return STATUS_CORRUPTED;

                vp->qsort();

                // Compress the data
                for (size_t j=0, m=vp->size(); j<m; ++j)
                {
                    io::Path *p = vp->uget(j);
                    if (p == NULL)
                        return STATUS_CORRUPTED;
                    if ((res = path.set(base, p)) != STATUS_OK)
                        return res;

                    // Open the source file
                    if ((res = ifs.open(&path)) != STATUS_OK)
                    {
                        fprintf(stderr, "Could not open file: %s\n", path.as_native());
                        return res;
                    }

                    // Compress the file
                    printf("  compressing file: %s\n", path.as_native());
                    if ((bytes = ctx->c.create_file(p, &ifs)) < 0)
                    {
                        fprintf(stderr, "Error compressing file: %s, code=%d\n",
                                path.as_native(), int(-bytes));
                        return -bytes;
                    }
                    ctx->in_bytes  += bytes;

                    // Close source file
                    if ((res = ifs.close()) != STATUS_OK)
                        return res;
                }

                // Flush the compressor
                if ((res = ctx->c.flush()) != STATUS_OK)
                    return res;
            }

            // Output statistics
            printf("Input size: %ld, compressed size: %ld, compression ratio: %.2f",
                    long(ctx->in_bytes), long(ctx->os->total()),
                    double(ctx->in_bytes) / double(ctx->os->total())
            );

            return STATUS_OK;
        }

        status_t write_entries(state_t *ctx)
        {
            FILE *fd = ctx->fd;

            // End of data[] array
            fprintf(fd, "\n\t};\n\n");

            // Start the entries description
            fprintf(fd, "\tstatic const raw_resource_t entries[] =\n");
            fprintf(fd, "\t{\n");

            const raw_resource_t *vent = ctx->c.entries();
            for (size_t i=0, n=ctx->c.num_entires(); i<n; ++i)
            {
                const raw_resource_t *r = &vent[i];

                if (i > 0)
                    fprintf(fd, ",\n");
                fprintf(fd, "\t\t/* %4d */ { %s, \"%s\", %ld, %ld, %ld, %ld }",
                        int(i),
                        (r->type == resource::RES_DIR) ? "RES_DIR" : "RES_FILE",
                        r->name, long(r->parent), long(r->segment), long(r->offset), long(r->length)
                );
            }

            // End of entries[] array
            fprintf(fd, "\n\t};\n\n");

            // Emit resource factory
            fprintf(fd, "\tcore::Resources builtin(data, %ld, entries, %ld);\n\n",
                    long(ctx->os->total()),
                    long(ctx->c.num_entires())
                );

            // End of anonymous namespace
            fprintf(fd, "}\n");

            return STATUS_OK;
        }

        status_t pack_tree(const char *destfile, const char *dir)
        {
            status_t res;
            io::Path path;
            resource::state_t *ctx = new resource::state_t();

            printf("Packing resources to %s from %s\n", destfile, dir);

            res     = path.set_native(dir);
            if (res == STATUS_OK)
                res     = scan_directory(ctx, &path, &path);
            if (res == STATUS_OK)
                res     = create_resource_file(ctx, destfile);
            if (res == STATUS_OK)
                res     = compress_data(ctx, &path);
            if (res == STATUS_OK)
                res     = write_entries(ctx);

            drop_context(ctx);

            return res;
        }
    }
}

#ifndef LSP_IDE_DEBUG
int main(int argc, const char **argv)
{
    if (argc < 3)
        fprintf(stderr, "USAGE: <dst-file> <src-dir>");
    lsp::status_t res = lsp::resource::pack_tree(argv[1], argv[2]);
    if (res != lsp::STATUS_OK)
        fprintf(stderr, "Error while generating resource file, code=%d", int(res));

    return res;
}
#endif /* LSP_IDE_DEBUG */
