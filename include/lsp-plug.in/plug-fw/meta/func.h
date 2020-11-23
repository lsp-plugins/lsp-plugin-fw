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

#ifndef LSP_PLUG_IN_PLUG_FW_META_FUNC_H_
#define LSP_PLUG_IN_PLUG_FW_META_FUNC_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

#define IS_OUT_PORT(p)      ((p)->flags & F_OUT)
#define IS_IN_PORT(p)       (!((p)->flags & F_OUT))
#define IS_GROWING_PORT(p)  (((p)->flags & (F_GROWING | F_UPPER | F_LOWER)) == (F_GROWING | F_UPPER | F_LOWER))
#define IS_LOWERING_PORT(p) (((p)->flags & (F_LOWERING | F_UPPER | F_LOWER)) == (F_LOWERING | F_UPPER | F_LOWER))
#define IS_TRIGGER_PORT(p)  ((p)->flags & F_TRG)

namespace lsp
{
    namespace plug
    {

        /**
         * Get name of the unit
         * @param unit unit_t unit
         * @return unit name (UTF-8 string), may be NULL
         */
        const char     *get_unit_name(size_t unit);

        /**
         * Get localized key for unit
         * @param unit unit_t unit
         * @return unit localized key, may be NULL
         */
        const char     *get_unit_lc_key(size_t unit);

        /**
         * Get unit by name
         * @param name unit name
         * @return unit, U_NONE if could not parse
         */
        unit_t          get_unit(const char *name);

        /**
         * Check that unit is of descrete type
         * @param unit unit_t unit
         * @return true if unit is of descrete type
         */
        bool            is_discrete_unit(size_t unit);

        /**
         * Check that unit is decibels
         * @param unit unit_t unit
         * @return true if unit is decibels
         */
        bool            is_decibel_unit(size_t unit);

        /**
         * Check that unit is gain
         * @param unit unit_t unit
         * @return true if unit is gain
         */
        bool            is_gain_unit(size_t unit);

        /**
         * Check that unit is degree unit
         * @param unit unit_t unit
         * @return true if unit is degree unit
         */
        bool            is_degree_unit(size_t unit);

        /**
         * Check that unit uses logarithmic rule
         * @param unit unit_t unit
         * @return true if unit uses logarithmic rule
         */
        bool            is_log_rule(const port_t *port);

        /**
         * Estimate the size of list (in elements) for the port type
         * @param list list to estimate size
         * @return size of list (in elements)
         */
        size_t          list_size(const port_item_t *list);

        /**
         * Limit floating-point value corresponding to the port's metadata
         * @param port port's metadata
         * @param value value to limit
         * @return limited value
         */
        float           limit_value(const port_t *port, float value);

        /**
         * Get all port parameters
         * @param p port metadata
         * @param min minimum value
         * @param max maximum value
         * @param step step
         */
        void            get_port_parameters(const port_t *p, float *min, float *max, float *step);

        /** Clone port metadata
         *
         * @param metadata port list
         * @param postfix potfix to be added to the port list, can be NULL
         * @return cloned port metadata, should be freed by drop_port_metadata() call
         */
        port_t         *clone_port_metadata(const port_t *metadata, const char *postfix);

        /** Drop port metadata
         *
         * @param metadata port metadata to drop
         */
        void            drop_port_metadata(port_t *metadata);

        /** Size of port list
         *
         * @param metadata port list metadata
         * @return number of elements excluding PORTS_END
         */
        size_t          port_list_size(const port_t *metadata);
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_META_FUNC_H_ */
