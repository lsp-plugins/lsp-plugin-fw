/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 23 дек. 2023 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_TEST_REPOSITORY_H_
#define PRIVATE_TEST_REPOSITORY_H_

#include <lsp-plug.in/io/Path.h>

namespace lsp
{
    namespace test
    {
        void make_repository(const io::Path *path);
    } /* namespace test */
} /* namespace lsp */


#endif /* MODULES_LSP_PLUGIN_FW_INCLUDE_PRIVATE_TEST_REPOSITORY_H_ */
