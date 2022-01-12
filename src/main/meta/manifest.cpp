/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 мая 2021 г.
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

#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/io/InSequence.h>
#include <lsp-plug.in/fmt/json/dom.h>
#include <lsp-plug.in/common/debug.h>
#include <errno.h>

namespace lsp
{
    namespace meta
    {
        static void drop_string(const char **str)
        {
            if (*str == NULL)
                return;

            free(const_cast<char *>(*str));
            *str    = NULL;
        }

        status_t load_manifest(meta::package_t **pkg, const char *path, const char *charset)
        {
            if ((pkg == NULL) || (path == NULL))
                return STATUS_BAD_ARGUMENTS;

            io::InFileStream ifs;
            status_t res = ifs.open(path);
            if (res == STATUS_OK)
                res     = load_manifest(pkg, &ifs, charset);
            status_t res2 = ifs.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t load_manifest(meta::package_t **pkg, const LSPString *path, const char *charset)
        {
            if ((pkg == NULL) || (path == NULL))
                return STATUS_BAD_ARGUMENTS;

            io::InFileStream ifs;
            status_t res = ifs.open(path);
            if (res == STATUS_OK)
                res     = load_manifest(pkg, &ifs, charset);
            status_t res2 = ifs.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t load_manifest(meta::package_t **pkg, const io::Path *path, const char *charset)
        {
            if ((pkg == NULL) || (path == NULL))
                return STATUS_BAD_ARGUMENTS;

            io::InFileStream ifs;
            status_t res = ifs.open(path);
            if (res == STATUS_OK)
                res     = load_manifest(pkg, &ifs, charset);
            status_t res2 = ifs.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t load_manifest(meta::package_t **pkg, io::IInStream *is, const char *charset)
        {
            if ((pkg == NULL) || (is == NULL))
                return STATUS_BAD_ARGUMENTS;

            io::InSequence ifs;
            status_t res    = ifs.wrap(is, 0, charset);
            if (res == STATUS_OK)
                res             = load_manifest(pkg, &ifs);
            status_t res2   = ifs.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t fetch_string(const char **s, const char *field, json::Object *jo)
        {
            LSPString str;
            json::String js = jo->get(field);
            if (!js.is_string())
            {
                lsp_error("manifest field '%s' expected to be of string type", field);
                return STATUS_CORRUPTED;
            }

            status_t res = js.get(&str);
            if (res != STATUS_OK)
            {
                lsp_error("could not fetch string value for manifest field '%s'", field);
                return res;
            }

            *s      = str.clone_utf8();
            if ((*s == NULL) && (str.length() > 0))
                return STATUS_NO_MEM;

            return STATUS_OK;
        }

        status_t fetch_version(lsp::version_t *v, const char *field, json::Object *jo)
        {
            LSPString str;
            json::String js = jo->get(field);
            if (!js.is_string())
            {
                lsp_error("manifest field '%s' expected to be of string type", field);
                return STATUS_CORRUPTED;
            }

            status_t res = js.get(&str);
            if (res != STATUS_OK)
            {
                lsp_error("could not fetch string value for manifest field '%s'", field);
                return res;
            }

            v->major    = 0;
            v->minor    = 0;
            v->micro    = 0;
            v->branch   = NULL;

            const char *xv = str.get_utf8();
            long val;
            char *e;
            errno = 0;

            printf("Parsing version: %s\n", xv);

            val = strtol(xv, &e, 10);

            if ((errno == 0) && (e > xv))
            {
                v->major    = val;
                printf("major = %d\n", int(v->major));
                if (*e == '.')
                {
                    xv = e + 1;
                    errno = 0;
                    val = strtol(xv, &e, 10);

                    if ((errno == 0) && (e > xv))
                    {
                        v->minor    = val;
                        printf("minor = %d\n", int(v->major));

                        if (*e == '.')
                        {
                            xv = e + 1;
                            errno = 0;
                            val = strtol(xv, &e, 10);

                            if ((errno == 0) && (e > xv))
                            {
                                v->micro    = val;
                                printf("micro = %d\n", int(v->major));
                            }
                        }
                    }
                }
            }

            // Branch?
            if (*e == '-')
            {
                v->branch   = strdup(e + 1);
                if (!v->branch)
                    return STATUS_NO_MEM;
                e += strlen(e);
            }

            // Should be end of line now
            if (*e != '\0')
            {
                printf("bad end of line\n");
                drop_string(&v->branch);
                return STATUS_BAD_FORMAT;
            }

            return STATUS_OK;
        }

        status_t load_manifest(meta::package_t **pkg, io::IInSequence *is)
        {
            json::Object jo;
            status_t res = json::dom_parse(is, &jo, json::JSON_LEGACY);
            if (res != STATUS_OK)
                return res;
            else if (!jo.is_object())
                return STATUS_CORRUPTED;

            package_t *p    = reinterpret_cast<package_t *>(malloc(sizeof(package_t)));
            if (p == NULL)
                return STATUS_NO_MEM;

            p->artifact         = NULL;
            p->artifact_name    = NULL;
            p->brand            = NULL;
            p->brand_id         = NULL;
            p->short_name       = NULL;
            p->full_name        = NULL;
            p->site             = NULL;
            p->email            = NULL;
            p->license          = NULL;
            p->lv2_license      = NULL;
            p->copyright        = NULL;
            p->version.major    = 0;
            p->version.minor    = 0;
            p->version.micro    = 0;
            p->version.branch   = NULL;

            if (res == STATUS_OK)
                res = fetch_string(&p->artifact, "artifact", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->artifact_name, "artifact_name", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->brand, "brand", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->brand_id, "brand_id", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->short_name, "short_name", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->full_name, "full_name", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->site, "site", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->email, "email", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->license, "license", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->lv2_license, "lv2_license", &jo);
            if (res == STATUS_OK)
                res = fetch_string(&p->copyright, "copyright", &jo);
            if (res == STATUS_OK)
            {
                printf("Parsing version\n");
                res = fetch_version(&p->version, "version", &jo);
                if (res != STATUS_OK)
                    fprintf(stderr, "Error parsing version\n");
            }

            if (res == STATUS_OK)
                *pkg            = p;

            return res;
        }

        void free_manifest(meta::package_t *pkg)
        {
            if (pkg == NULL)
                return;

            drop_string(&pkg->artifact);
            drop_string(&pkg->artifact_name);
            drop_string(&pkg->brand);
            drop_string(&pkg->brand_id);
            drop_string(&pkg->short_name);
            drop_string(&pkg->full_name);
            drop_string(&pkg->site);
            drop_string(&pkg->license);
            drop_string(&pkg->copyright);
            drop_string(&pkg->version.branch);
            free(pkg);
        }
    }
}


