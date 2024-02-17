/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 28 дек. 2022 г.
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

#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/dsp-units/units.h>

UTEST_BEGIN("meta.func", parse)

    void test_parse_bool()
    {
        static const meta::port_t port = {
            "test",
            "Test BOOL port",
            meta::U_BOOL,
            meta::R_CONTROL,
            0,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            NULL,
            NULL
        };

        for (size_t i=0; i<2; ++i)
        {
            bool units = i > 0;

            printf("Testing boolean parse units=%s\n", (units) ? "true" : "false");

            // Test valid cases
            float dst = -1.0f;
            UTEST_ASSERT(meta::parse_value(&dst, "true", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "false", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " TRUE ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "  FaLsE  ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " T ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " F ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " ON ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " OFF ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " 1.0 ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " 0.1 ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " -1.0 ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " -0.1 ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " YES ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " NO ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));

            // Test invalid cases
            dst = -1.0f;
            UTEST_ASSERT(meta::parse_value(&dst, "blablabla", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "tru", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "0 dB", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

            // Test empty cases
            UTEST_ASSERT(meta::parse_value(&dst, "", &port, units) == STATUS_BAD_ARGUMENTS);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, units) == STATUS_BAD_ARGUMENTS);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        }
    }

    void test_parse_enum()
    {
        static const meta::port_item_t enum_values[] =
        {
            { "red", "red", },
            { "green", "green" },
            { "blue", "blue" },
            { NULL, NULL }
        };

        static const meta::port_t port = {
            "test",
            "Test ENUM port",
            meta::U_ENUM,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_STEP,
            2.0f,
            0.0f,
            2.0f,
            3.0f,
            enum_values,
            NULL
        };

        for (size_t i=0; i<2; ++i)
        {
            bool units = i > 0;

            printf("Testing enumeration parse units=%s\n", (units) ? "true" : "false");

            // Test valid cases
            float dst = -1.0f;
            UTEST_ASSERT(meta::parse_value(&dst, "red", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 2.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "green", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 5.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "blue", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 8.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "  RED  ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 2.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "  GREEN  ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 5.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "  BLUE  ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 8.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " 2.0 ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 2.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " 5.0 ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 5.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " 8.0 ", &port, units) == STATUS_OK);
            UTEST_ASSERT(float_equals_adaptive(dst, 8.0f));

            // Test invalid cases
            dst = -1.0f;
            UTEST_ASSERT(meta::parse_value(&dst, "r", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "reds", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "redgreen", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "g", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "b", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "3", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "4.0", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "0 dB", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

            // Test empty cases
            UTEST_ASSERT(meta::parse_value(&dst, "", &port, units) == STATUS_BAD_ARGUMENTS);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, units) == STATUS_BAD_ARGUMENTS);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        }
    };

    void test_parse_gain_amp()
    {
        static const meta::port_t port = {
            "test",
            "Test GAIN_AMP port",
            meta::U_GAIN_AMP,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            0.0f,
            1000.0f,
            1.0f,
            0.01f,
            NULL,
            NULL
        };

        //------------------------------------------------
        printf("Testing gain_amp parse units=false\n");

        // Test valid cases
        float dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0.0", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_gain(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_gain(-10.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -inf ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0 dB", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        //------------------------------------------------
        printf("Testing gain_amp parse units=true\n");

        // Test valid cases
        UTEST_ASSERT(meta::parse_value(&dst, "0.0 dB", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_gain(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 dB  ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_gain(-10.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -inf dB", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 0.0", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_gain(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_gain(-10.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " 0.5 G ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.5f));
        UTEST_ASSERT(meta::parse_value(&dst, " 0 Np ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::neper_to_gain(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 Np ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::neper_to_gain(-10.0f)));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0 bar", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
    };

    void test_parse_gain_pow()
    {
        static const meta::port_t port = {
            "test",
            "Test GAIN_AMP port",
            meta::U_GAIN_POW,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            0.0f,
            1000.0f,
            1.0f,
            0.01f,
            NULL,
            NULL
        };

        //------------------------------------------------
        printf("Testing gain_pow parse units=false\n");

        // Test valid cases
        float dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0.0", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_power(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_power(-10.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -inf ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0 dB", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        //------------------------------------------------
        printf("Testing gain_pow parse units=true\n");

        // Test valid cases
        UTEST_ASSERT(meta::parse_value(&dst, "0.0 dB", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_power(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 dB  ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_power(-10.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -inf dB", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 0.0", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_power(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_power(-10.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " 0.5 G ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.5f));
        UTEST_ASSERT(meta::parse_value(&dst, " 0 Np ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::neper_to_power(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 Np ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::neper_to_power(-10.0f)));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0 bar", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
    };

    void test_parse_db()
    {
        static const meta::port_t port = {
            "test",
            "Test DB port",
            meta::U_DB,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            -60.0f,
            60.0f,
            0.0f,
            0.1f,
            NULL,
            NULL
        };

        //------------------------------------------------
        printf("Testing db parse units=false\n");

        // Test valid cases
        float dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0.0", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0 dB", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        //------------------------------------------------
        printf("Testing db parse units=true\n");

        // Test valid cases
        UTEST_ASSERT(meta::parse_value(&dst, "0.0 dB", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 dB  ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 0.0", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 0.5 G ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::gain_to_db(0.5f)));
        UTEST_ASSERT(meta::parse_value(&dst, " 0 Np ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::neper_to_db(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 Np ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::neper_to_db(-10.0f)));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0 bar", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
    };

    void test_parse_neper()
    {
        static const meta::port_t port = {
            "test",
            "Test NEPER port",
            meta::U_NEPER,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            -60.0f,
            60.0f,
            0.0f,
            0.1f,
            NULL,
            NULL
        };

        //------------------------------------------------
        printf("Testing db parse units=false\n");

        // Test valid cases
        float dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0.0", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0 Np", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        //------------------------------------------------
        printf("Testing db parse units=true\n");

        // Test valid cases
        UTEST_ASSERT(meta::parse_value(&dst, "0.0 dB", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_neper(0.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 dB  ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::db_to_neper(-10.0f)));
        UTEST_ASSERT(meta::parse_value(&dst, " 0.0", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 0.5 G ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, dspu::gain_to_neper(0.5f)));
        UTEST_ASSERT(meta::parse_value(&dst, " 0 Np ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 0.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 Np ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "0 bar", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
    };

    void test_parse_note()
    {
        static const meta::port_t port = {
            "test",
            "Test FREQUENCY port",
            meta::U_HZ,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            0.0f,
            24000.0f,
            0.0f,
            0.1f,
            NULL,
            NULL
        };

        for (size_t i=0; i<2; ++i)
        {
            bool units = i > 0;

            printf("Testing note parse units=%s\n", (units) ? "true" : "false");

            // Test valid cases
            float dst = -1.0f;
            UTEST_ASSERT(meta::parse_value(&dst, "a", &port, units) == STATUS_OK); // A4
            UTEST_ASSERT(float_equals_adaptive(dst, 440.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " a2 ", &port, units) == STATUS_OK); // A2
            UTEST_ASSERT(float_equals_adaptive(dst, 110.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " A 3 ", &port, units) == STATUS_OK); // A3
            UTEST_ASSERT(float_equals_adaptive(dst, 220.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " A-1 ", &port, units) == STATUS_OK); // A-1 = MIDI #9
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(9)));
            UTEST_ASSERT(meta::parse_value(&dst, " C-1 ", &port, units) == STATUS_OK); // C-1 = MIDI #0
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(0)));
            UTEST_ASSERT(meta::parse_value(&dst, " G9 ", &port, units) == STATUS_OK);  // C9 = MIDI #127
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(127)));

            UTEST_ASSERT(meta::parse_value(&dst, " a# ", &port, units) == STATUS_OK); // A#4 = MIDI #70
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(70)));
            UTEST_ASSERT(meta::parse_value(&dst, " a## ", &port, units) == STATUS_OK); // A##4 = MIDI #71
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(71)));
            UTEST_ASSERT(meta::parse_value(&dst, " ab ", &port, units) == STATUS_OK); // Ab4 = MIDI #69
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(68)));
            UTEST_ASSERT(meta::parse_value(&dst, " abb ", &port, units) == STATUS_OK); // Abb4 = MIDI #68
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(67)));

            UTEST_ASSERT(meta::parse_value(&dst, " a#3 ", &port, units) == STATUS_OK); // A#3 = MIDI #58
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(58)));
            UTEST_ASSERT(meta::parse_value(&dst, " a##3 ", &port, units) == STATUS_OK); // A##3 = MIDI #59
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(59)));
            UTEST_ASSERT(meta::parse_value(&dst, " ab3 ", &port, units) == STATUS_OK); // Ab3 = MIDI #57
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(56)));
            UTEST_ASSERT(meta::parse_value(&dst, " abb3 ", &port, units) == STATUS_OK); // Abb3 = MIDI #56
            UTEST_ASSERT(float_equals_adaptive(dst, dspu::midi_note_to_frequency(55)));

            // Test invalid cases
            dst = -1.0f;
            UTEST_ASSERT(meta::parse_value(&dst, "a Hz", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "a kHz", &port, units) == STATUS_INVALID_VALUE);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "G#9", &port, units) == STATUS_INVALID_VALUE); // Out of MIDI note
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "B-2", &port, units) == STATUS_INVALID_VALUE); // Out of MIDI note
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, " Cb-1 ", &port, units) == STATUS_INVALID_VALUE); // Out of MIDI note
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "Abbb", &port, units) == STATUS_INVALID_VALUE); // No triple alteration
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "A###", &port, units) == STATUS_INVALID_VALUE); // No triple alteration
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

            // Test empty cases
            dst = -1.0f;
            UTEST_ASSERT(meta::parse_value(&dst, "", &port, units) == STATUS_BAD_ARGUMENTS);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
            UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, units) == STATUS_BAD_ARGUMENTS);
            UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        }
    };

    void test_parse_freq()
    {
        static const meta::port_t port = {
            "test",
            "Test FREQUENCY port",
            meta::U_KHZ,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            0.0f,
            24000.0f,
            0.0f,
            0.1f,
            NULL,
            NULL
        };

        //------------------------------------------------
        printf("Testing frequency parse units=false\n");

        // Test valid cases
        float dst = -1.0f;

        UTEST_ASSERT(meta::parse_value(&dst, "a", &port, false) == STATUS_OK); // A4
        UTEST_ASSERT(float_equals_adaptive(dst, 0.440f));
        UTEST_ASSERT(meta::parse_value(&dst, "a3", &port, false) == STATUS_OK); // A3
        UTEST_ASSERT(float_equals_adaptive(dst, 0.220f));
        UTEST_ASSERT(meta::parse_value(&dst, "1", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "1000.0", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1000.0f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "2.0 2.0", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        UTEST_ASSERT(meta::parse_value(&dst, "2u", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2m", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2k", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2M", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2G", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        UTEST_ASSERT(meta::parse_value(&dst, "2uhz", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2mhz", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2hz", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2khz", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2Mhz", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2Ghz", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        UTEST_ASSERT(meta::parse_value(&dst, " 3 uhz ", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 mhz ", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 hz ", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 khz ", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 Mhz ", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 Ghz ", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        //------------------------------------------------
        printf("Testing frequency parse units=true\n");

        // Test valid cases
        dst = -1.0f;

        UTEST_ASSERT(meta::parse_value(&dst, "a", &port, true) == STATUS_OK); // A4
        UTEST_ASSERT(float_equals_adaptive(dst, 0.440f));
        UTEST_ASSERT(meta::parse_value(&dst, "a3", &port, true) == STATUS_OK); // A3
        UTEST_ASSERT(float_equals_adaptive(dst, 0.220f));
        UTEST_ASSERT(meta::parse_value(&dst, "1", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "1000.0", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1000.0f));

        UTEST_ASSERT(meta::parse_value(&dst, "2u", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2e-9f));
        UTEST_ASSERT(meta::parse_value(&dst, "2m", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1e-6f));
        UTEST_ASSERT(meta::parse_value(&dst, "2k", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2M", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2e+3f));
        UTEST_ASSERT(meta::parse_value(&dst, "2G", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2e+6f));

        UTEST_ASSERT(meta::parse_value(&dst, "2uhz", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2e-9f));
        UTEST_ASSERT(meta::parse_value(&dst, "2mhz", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1e-6f));
        UTEST_ASSERT(meta::parse_value(&dst, "2hz", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2e-3f));
        UTEST_ASSERT(meta::parse_value(&dst, "2khz", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2Mhz", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2e+3f));
        UTEST_ASSERT(meta::parse_value(&dst, "2Ghz", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2e+6f));

        UTEST_ASSERT(meta::parse_value(&dst, " 3 uhz ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 3e-9f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 mhz ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 3e-6f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 hz ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 3e-3f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 khz ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 3.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 Mhz ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 3e+3f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 Ghz ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 3e+6f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "2.0 2.0", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        UTEST_ASSERT(meta::parse_value(&dst, "2u hz", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2m hz", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2h z", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2k hz", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2M hz", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2G hz", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        UTEST_ASSERT(meta::parse_value(&dst, " 3 u hz ", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 m hz ", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 k hz ", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 M hz ", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " 3 G hz ", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
    };

    void test_parse_int()
    {
        static const meta::port_t port = {
            "test",
            "Test BAR port",
            meta::U_BAR,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP | meta::F_INT,
            0.0f,
            1000.0f,
            0.0f,
            1.0f,
            NULL,
            NULL
        };

        //------------------------------------------------
        printf("Testing int parse units=false\n");

        // Test valid cases
        float dst = -1.0f;

        UTEST_ASSERT(meta::parse_value(&dst, "1", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "42", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 42.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1 2", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "1 bar", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "-1 bar", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "0 db", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        //------------------------------------------------
        printf("Testing int parse units=true\n");

        // Test valid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "42", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 42.0f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10 ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "10 bar", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 10.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "-10 bar", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.0f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1 2", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "0 db", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
    };

    void test_parse_float()
    {
        static const meta::port_t port = {
            "test",
            "Test BAR port",
            meta::U_BAR,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            0.0f,
            1000.0f,
            0.0f,
            1.0f,
            NULL,
            NULL
        };

        //------------------------------------------------
        printf("Testing float parse units=false\n");

        // Test valid cases
        float dst = -1.0f;

        UTEST_ASSERT(meta::parse_value(&dst, "1.5", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1.5f));
        UTEST_ASSERT(meta::parse_value(&dst, "42.5", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 42.5f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10.5 ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.5f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1 2", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "1 bar", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "-1 bar", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "0 db", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        //------------------------------------------------
        printf("Testing float parse units=true\n");

        // Test valid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1.5", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1.5f));
        UTEST_ASSERT(meta::parse_value(&dst, "42.5", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 42.5f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10.5 ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.5f));
        UTEST_ASSERT(meta::parse_value(&dst, "10.5 bar", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 10.5f));
        UTEST_ASSERT(meta::parse_value(&dst, "-10.5 bar", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.5f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1 2", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "0 db", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
    };

    void test_parse_time()
    {
        static const meta::port_t port = {
            "test",
            "Test SEC port",
            meta::U_SEC,
            meta::R_CONTROL,
            meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            0.0f,
            1000.0f,
            0.0f,
            1.0f,
            NULL,
            NULL
        };

        //------------------------------------------------
        printf("Testing float parse units=false\n");

        // Test valid cases
        float dst = -1.0f;

        UTEST_ASSERT(meta::parse_value(&dst, "1.5", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1.5f));
        UTEST_ASSERT(meta::parse_value(&dst, "42.5", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 42.5f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10.5 ", &port, false) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.5f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1 2", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "1 s", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "-1 s", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "0 db", &port, false) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, false) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        //------------------------------------------------
        printf("Testing float parse units=true\n");

        // Test valid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1.5", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 1.5f));
        UTEST_ASSERT(meta::parse_value(&dst, "42.5", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 42.5f));
        UTEST_ASSERT(meta::parse_value(&dst, " -10.5 ", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.5f));
        UTEST_ASSERT(meta::parse_value(&dst, "10.5 s", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 10.5f));
        UTEST_ASSERT(meta::parse_value(&dst, "-10.5 s", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, -10.5f));

        UTEST_ASSERT(meta::parse_value(&dst, "2 min", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 120.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2 s", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "2ms", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2.0e-3f));
        UTEST_ASSERT(meta::parse_value(&dst, "2us", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2.0e-6f));
        UTEST_ASSERT(meta::parse_value(&dst, "2ns", &port, true) == STATUS_OK);
        UTEST_ASSERT(float_equals_adaptive(dst, 2.0e-9f));

        // Test invalid cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "1 2", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "0 db", &port, true) == STATUS_INVALID_VALUE);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));

        // Test empty cases
        dst = -1.0f;
        UTEST_ASSERT(meta::parse_value(&dst, "", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
        UTEST_ASSERT(meta::parse_value(&dst, "    ", &port, true) == STATUS_BAD_ARGUMENTS);
        UTEST_ASSERT(float_equals_adaptive(dst, -1.0f));
    };

    UTEST_MAIN
    {
        test_parse_bool();
        test_parse_enum();
        test_parse_gain_amp();
        test_parse_gain_pow();
        test_parse_db();
        test_parse_neper();
        test_parse_note();
        test_parse_freq();
        test_parse_int();
        test_parse_float();
        test_parse_time();
    }

UTEST_END


