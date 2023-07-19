/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 28 сент. 2015 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_META_TYPES_H_
#define LSP_PLUG_IN_PLUG_FW_META_TYPES_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/types.h>

#define LSP_MODULE_VERSION(a, b, c)             ::lsp::meta::module_version_t{ (a), (b), (c) }
#define LSP_MODULE_VERSION_MAJOR(v)             ((v).major)
#define LSP_MODULE_VERSION_MINOR(v)             ((v).minor)
#define LSP_MODULE_VERSION_MICRO(v)             ((v).micro)

namespace lsp
{
    namespace meta
    {
        enum unit_t
        {
            U_NONE,                 // Simple value

            U_BOOL,                 // Boolean: true if > 0.5, false otherwise
            U_STRING,               // String
            U_PERCENT,              // Something in percents

            // Distance
            U_MM,                   // Millimeters
            U_CM,                   // Centimeters
            U_M,                    // Meters
            U_INCH,                 // Inches
            U_KM,                   // Kilometers

            // Speed
            U_MPS,                  // Meters per second
            U_KMPH,                 // Kilometers per hour

            // Samples
            U_SAMPLES,              // Something in samples

            // Frequency
            U_HZ,                   // Hertz
            U_KHZ,                  // Kilohertz
            U_MHZ,                  // Megahertz
            U_BPM,                  // Beats per minute
            U_CENT,                 // Cents
            U_OCTAVES,              // Octaves
            U_SEMITONES,            // Semitones

            // Time measurement
            U_BAR,                  // Bars
            U_BEAT,                 // Beats
            U_MIN,                  // Minute
            U_SEC,                  // Seconds
            U_MSEC,                 // Milliseconds

            // Level measurement
            U_DB,                   // Decibels
            U_GAIN_AMP,             // Gain (amplitude amplification)
            U_GAIN_POW,             // Gain (power amplification)
            U_NEPER,                // Nepers

            // Degrees
            U_DEG,                  // Degrees
            U_DEG_CEL,              // Degrees (Celsium)
            U_DEG_FAR,              // Degrees (Fahrenheit)
            U_DEG_K,                // Degrees (Kelvin)
            U_DEG_R,                // Degrees (Rankine)

            // Size units
            U_BYTES,                // Bytes
            U_KBYTES,               // Kilobytes
            U_MBYTES,               // Megabytes
            U_GBYTES,               // Gigabytes
            U_TBYTES,               // Terabytes

            U_ENUM                  // List index
        };

        enum role_t
        {
            R_UI_SYNC,              // Synchronization with UI
            R_AUDIO,                // Audio port
            R_CONTROL,              // Control port
            R_METER,                // Metering port
            R_MESH,                 // Mesh port
            R_FBUFFER,              // Frame buffer
            R_PATH,                 // Path to the local file
            R_MIDI,                 // MIDI events
            R_PORT_SET,             // Set of ports
            R_OSC,                  // OSC events
            R_BYPASS,               // Bypass
            R_STREAM                // Stream
        };

        enum flags_t
        {
            F_IN            = (0 << 0),     // Input port
            F_OUT           = (1 << 0),     // Output port
            F_UPPER         = (1 << 1),     // Upper-limit defined
            F_LOWER         = (1 << 2),     // Lower-llmit defined
            F_STEP          = (1 << 3),     // Step defined
            F_LOG           = (1 << 4),     // Logarithmic scale
            F_INT           = (1 << 5),     // Integer value
            F_TRG           = (1 << 6),     // Trigger
            F_GROWING       = (1 << 7),     // Proportionally growing default value (for port sets)
            F_LOWERING      = (1 << 8),     // Proportionally lowering default value (for port sets)
            F_PEAK          = (1 << 9),     // Peak flag
            F_CYCLIC        = (1 << 10),    // Cyclic flag
            F_EXT           = (1 << 11),    // Extended range
        };

        enum plugin_class_t
        {
            C_DELAY,
                C_REVERB,
            C_DISTORTION,
                C_WAVESHAPER,
            C_DYNAMICS,
                C_AMPLIFIER,
                C_COMPRESSOR,
                C_ENVELOPE,
                C_EXPANDER,
                C_GATE,
                C_LIMITER,
            C_FILTER,
                C_ALLPASS,
                C_BANDPASS,
                C_COMB,
                C_EQ,
                    C_MULTI_EQ,
                    C_PARA_EQ,
                C_HIGHPASS,
                C_LOWPASS,
            C_GENERATOR,
                C_CONSTANT,
                C_INSTRUMENT,
                C_OSCILLATOR,
            C_MODULATOR,
                C_CHORUS,
                C_FLANGER,
                C_PHASER,
            C_SIMULATOR,
            C_SPATIAL,
            C_SPECTRAL,
                C_PITCH,
            C_UTILITY,
                C_ANALYSER,
                C_CONVERTER,
                C_FUNCTION,
                C_MIXER,

            // Overall number of classes
            C_TOTAL
        };

        enum clap_feature_t
        {
            // Primary plugin catetory
            CF_INSTRUMENT,
            CF_AUDIO_EFFECT,
            CF_NOTE_EFFECT,
            CF_ANALYZER,

            // Plugin sub-category
            CF_SYNTHESIZER,
            CF_SAMPLER,
            CF_DRUM,
            CF_DRUM_MACHINE,

            CF_FILTER,
            CF_PHASER,
            CF_EQUALIZER,
            CF_DEESSER,
            CF_PHASE_VOCODER,
            CF_GRANULAR,
            CF_FREQUENCY_SHIFTER,
            CF_PITCH_SHIFTER,

            CF_DISTORTION,
            CF_TRANSIENT_SHAPER,
            CF_COMPRESSOR,
            CF_LIMITER,

            CF_FLANGER,
            CF_CHORUS,
            CF_DELAY,
            CF_REVERB,

            CF_TREMOLO,
            CF_GLITCH,

            CF_UTILITY,
            CF_PITCH_CORRECTION,
            CF_RESTORATION,

            CF_MULTI_EFFECTS,

            CF_MIXING,
            CF_MASTERING,

            // Audio Capabilities
            CF_MONO,
            CF_STEREO,
            CF_SURROUND,
            CF_AMBISONIC,

            // Overall number of features
            CF_TOTAL
        };

        /**
         * Different bundle groups
         */
        enum bundle_group_t
        {
            B_ANALYZERS,
            B_CONVOLUTION,
            B_DELAYS,
            B_DYNAMICS,
            B_EQUALIZERS,
            B_EFFECTS,
            B_MB_DYNAMICS,
            B_MB_PROCESSING,
            B_SAMPLERS,
            B_GENERATORS,
            B_UTILITIES
        };

        enum plugin_extension_t
        {
            E_NONE                  = 0,        // No extensions
            E_INLINE_DISPLAY        = 1 << 0,   // Supports InlineDisplay extension originally implemented in LV2 plugin format
            E_3D_BACKEND            = 1 << 1,   // Supports 3D rendering backend
            E_OSC                   = 1 << 2,   // Supports OSC protocol messaging
            E_KVT_SYNC              = 1 << 3,   // KVT synchronization required
            E_DUMP_STATE            = 1 << 4,   // Support of internal state dump
            E_FILE_PREVIEW          = 1 << 5,   // Support of internal state dump
        };

        enum port_group_type_t
        {
            GRP_MONO,                       // Mono
            GRP_1_0     = GRP_MONO,         // Mono
            GRP_STEREO,                     // Stereo
            GRP_2_0     = GRP_STEREO,       // Stereo
            GRP_2_1     = GRP_STEREO,       // Stereo
            GRP_MS,                         // Mid-side
            GRP_3_0,                        // 3.0
            GRP_4_0,                        // 4.0
            GRP_5_0,                        // 5.0
            GRP_5_1,                        // 5.1
            GRP_6_1,                        // 6.1
            GRP_7_1,                        // 7.1
            GRP_7_1W,                       // 7.1 Wide
        };

        enum port_group_role_t
        {
            PGR_CENTER,
            PGR_CENTER_LEFT,
            PGR_CENTER_RIGHT,
            PGR_LEFT,
            PGR_LO_FREQ,
            PGR_REAR_CENTER,
            PGR_REAR_LEFT,
            PGR_REAR_RIGHT,
            PGR_RIGHT,
            PGR_SIDE_LEFT,
            PGR_SIDE_RIGHT,
            PGR_MS_SIDE,
            PGR_MS_MIDDLE
        };

        enum port_group_flags_t
        {
            PGF_IN          = (0 << 0),     // Input group
            PGF_OUT         = (1 << 0),     // Output group
            PGF_SIDECHAIN   = (1 << 1),     // Sidechain
            PGF_MAIN        = (1 << 2),     // Main input/output group
        };

        typedef uint32_t            version_t;

        typedef struct module_version_t
        {
            uint8_t             major;
            uint8_t             minor;
            uint8_t             micro;
        } module_version_t;

        /**
         * The item of the port group
         */
        typedef struct port_group_item_t
        {
            const char         *id;
            port_group_role_t   role;
        } port_group_item_t;

        /**
         * Port group
         */
        typedef struct port_group_t
        {
            const char                 *id;         // Group ID
            const char                 *name;       // Group name
            port_group_type_t           type;       // Group type
            int                         flags;
            const port_group_item_t    *items;
            const char                 *parent_id;  // Reference to parent group
        } port_group_t;

        /**
         * For ports present like lists: the item of the list
         */
        typedef struct port_item_t
        {
            const char             *text;           // Text to display, required
            const char             *lc_key;         // Localized key  (optional)
        } port_item_t;

        /**
         * Plugin input/opuput port descriptor
         */
        typedef struct port_t
        {
            const char             *id;             // Control ID
            const char             *name;           // Control name
            unit_t                  unit;           // Units
            role_t                  role;           // Role
            int                     flags;          // Flags
            float                   min;            // Minimum value
            float                   max;            // Maximum value
            float                   start;          // Initial value
            float                   step;           // Change step
            const port_item_t      *items;          // Items for enum / port set
            const port_t           *members;        // Port members for group
        } port_t;

        /**
         * Medatata for developer/maintainer information
         */
        typedef struct person_t
        {
            const char             *uid;            // UID of person
            const char             *nick;           // Nickname
            const char             *name;           // Name
            const char             *mailbox;        // E-mail
            const char             *homepage;       // Homepage
        } person_t;

        /**
         * Metadata for plugin bundle
         */
        typedef struct package_t
        {
            const char             *artifact;       // Artifact name - string identifier (UTF-8)
            const char             *artifact_name;  // Artifact name - full artifact name (UTF-8)
            const char             *brand;          // Brand name
            const char             *brand_id;       // Brand identifier for serialization (LV2 TTL, for example)
            const char             *short_name;     // Sort name/Acronym
            const char             *full_name;      // Full name
            const char             *site;           // Site URL
            const char             *email;          // Email
            const char             *license;        // License
            const char             *lv2_license;    // License URI for LV2
            const char             *copyright;      // Copyright
            lsp::version_t          version;        // Package version
        } package_t;

        /**
         * Plugin bundle
         */
        typedef struct bundle_t
        {
            const char             *uid;            // Unique bundle identifier
            const char             *name;           // Bundle name
            bundle_group_t          group;          // Bundle group
            const char             *video_id;       // Bundle video identifier
            const char             *description;    // Bundle description
        } bundle_t;

        /**
         * Metadata for plugin instance classs
         */
        typedef struct plugin_t
        {
            const char             *name;           // Plugin name
            const char             *description;    // Plugin description
            const char             *acronym;        // Plugin acronym
            const person_t         *developer;      // Developer
            const char             *uid;            // Unique character identifier of plugin
            const char             *lv2_uri;        // LV2 URI
            const char             *lv2ui_uri;      // LV2 UI URI
            const char             *vst2_uid;       // Steinberg VST 2.x ID of the plugin
            const uint32_t          ladspa_id;      // LADSPA ID of the plugin
            const char             *ladspa_lbl;     // LADSPA unique label of the plugin
            const char             *clap_uid;       // Unique identifier for CLAP format
            const module_version_t  version;        // Version of the plugin
            const int              *classes;        // List of plugin classes terminated by negative value
            const int              *clap_features;  // List of CLAP plugin features
            const int               extensions;     // Additional extensions
            const port_t           *ports;          // List of all ports
            const char             *ui_resource;    // Location of the UI file resource
            const char             *ui_presets;     // Prefix of the preset location
            const port_group_t     *port_groups;    // List of all port groups
            const bundle_t         *bundle;         // Bundle associated with plugin
        } plugin_t;

    } /* namespace meta */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_META_TYPES_H_ */
