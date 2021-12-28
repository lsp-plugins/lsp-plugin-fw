/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 дек. 2021 г.
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

#include <lsp-plug.in/plug-fw/util/common/checksum.h>

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/fmt/json/Serializer.h>

namespace lsp
{
    namespace util
    {
        bool match_checksum(const checksum_t *c1, const checksum_t *c2)
        {
            return (c1->ck1 == c2->ck1) &&
                   (c1->ck2 == c2->ck2) &&
                   (c1->ck3 == c2->ck3);
        }

        status_t calc_checksum(checksum_t *dst, const io::Path *file)
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

        status_t add_checksum(checksum_list_t *list, const io::Path *base, const io::Path *file)
        {
            status_t res;
            io::Path path;

            if ((res = path.set(file)) != STATUS_OK)
                return res;
            if (base != NULL)
            {
                if ((res = path.remove_base(base)) != STATUS_OK)
                    return res;
            }
            const LSPString *key = path.as_string();
            if (key == NULL)
                return STATUS_NO_MEM;

            checksum_t *cksum = reinterpret_cast<checksum_t *>(malloc(sizeof(checksum_t)));
            if (cksum == NULL)
                return STATUS_NO_MEM;

            if ((res = calc_checksum(cksum, file)) == STATUS_OK)
            {
                if (list->create(key, cksum))
                    return STATUS_OK;
                res = STATUS_NO_MEM;
            }

            free(cksum);
            return res;
        }

        static status_t do_save_checksums(checksum_list_t *list, json::Serializer *s)
        {
            char buf[0x20];
            status_t res;
            lltl::parray<LSPString> keys;
            if ((res = s->start_object()) != STATUS_OK)
                return res;

            // Form the list of files
            if (!list->keys(&keys))
                return STATUS_NO_MEM;
            keys.qsort();

            // Generate key <=> value checksums
            for (size_t i=0, n=keys.size(); i<n; ++i)
            {
                LSPString *key = keys.uget(i);
                if (key == NULL)
                    return STATUS_CORRUPTED;
                checksum_t *cksum = list->get(key);
                if (cksum == NULL)
                    return STATUS_CORRUPTED;

                // Emit the list of checksums
                if ((res = s->write_property(key)) != STATUS_OK)
                    return res;
                if ((res = s->start_array()) != STATUS_OK)
                    return res;

                snprintf(buf, sizeof(buf)-1, "%016llx", (unsigned long long)(cksum->ck1));
                if ((res = s->write_string(buf)))
                    return res;
                snprintf(buf, sizeof(buf)-1, "%016llx", (unsigned long long)(cksum->ck2));
                if ((res = s->write_string(buf)))
                    return res;
                snprintf(buf, sizeof(buf)-1, "%016llx", (unsigned long long)(cksum->ck3));
                if ((res = s->write_string(buf)))
                    return res;

                if ((res = s->end_array()) != STATUS_OK)
                    return res;
            }


            if ((res = s->end_object()) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        status_t save_checksums(checksum_list_t *list, const io::Path *file)
        {
            status_t res, res2;
            json::Serializer s;
            json::serial_flags_t flags;

            // Create serializer
            json::init_serial_flags(&flags);
            flags.multiline = true;
            flags.padding = 1;
            flags.ident = '\t';
            if ((res = s.open(file, &flags, "UTF-8")) != STATUS_OK)
                return res;

            // Write items
            res = do_save_checksums(list, &s);
            res2 = s.close();

            return (res == STATUS_OK) ? res2 : res;
        }

        status_t drop_checksums(checksum_list_t *list)
        {
            lltl::parray<checksum_t> values;
            if (!list->values(&values))
                return STATUS_NO_MEM;
            list->flush();

            for (size_t i=0, n=values.size(); i<n; ++i)
            {
                checksum_t *cksum = values.uget(i);
                if (cksum != NULL)
                    free(cksum);
            }
            values.flush();

            return STATUS_OK;
        }

    } /* namespace util */
} /* namespace lsp */


