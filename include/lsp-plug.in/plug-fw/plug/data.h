/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_PLUG_DATA_H_
#define LSP_PLUG_IN_PLUG_FW_PLUG_DATA_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/protocol/midi.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/stdlib/string.h>

#define STREAM_MESH_ALIGN           0x40
#define STREAM_MAX_FRAME_SIZE       0x2000
#define STREAM_BULK_MAX             0x40
#define FRAMEBUFFER_BULK_MAX        0x10
#define MESH_REFRESH_RATE           20

#ifdef PLATFORM_WINDOWS
    #define MAX_PATH_LEN                (PATH_MAX * 2)
#else
    #define MAX_PATH_LEN                PATH_MAX
#endif /* PLATFORM_WINDOWS */

namespace lsp
{
    namespace plug
    {
        enum mesh_state_t
        {
            M_WAIT,         // Mesh is waiting for data request
            M_EMPTY,        // Mesh is empty
            M_DATA          // Mesh contains data
        };

        // Mesh port structure
        typedef struct mesh_t
        {
            volatile mesh_state_t   nState;     // Actual state of the mesh
            size_t                  nBuffers;   // Overall number of buffers
            size_t                  nItems;     // Number of items per each buffer
            float                  *pvData[];   // Array of pointers to buffer data

            inline bool isEmpty() const         { return nState == M_EMPTY; };
            inline bool containsData() const    { return nState == M_DATA; };
            inline bool isWaiting() const       { return nState == M_WAIT;  };

            inline void data(size_t bufs, size_t items)
            {
                nBuffers    = bufs;
                nItems      = items;
                nState      = M_DATA; // This should be the last operation
            }

            inline void cleanup()
            {
                nBuffers    = 0;
                nItems      = 0;
                nState      = M_EMPTY; // This should be the last operation
            }

            inline void markEmpty()
            {
                nState      = M_EMPTY; // This should be the last operation
            }

            inline void setWaiting()
            {
                nState      = M_WAIT; // This should be the last operation
            }
        } mesh_t;

        /**
         * Streaming mesh. The data structure consists from a long single buffer splitted into
         * channels. The mesh is incrementally appended with special markers - frames. Each frame
         * contains information about it's size, position inside of the buffer and the overall
         * buffer size available for read. Such structure allows to implement some kind of LIFO
         * with support of partial data read and transfer.
         */
        typedef struct stream_t
        {
            protected:
                typedef struct frame_t
                {
                    volatile uint32_t   id;         // Unique frame identifier
                    size_t              head;       // Head of the frame
                    size_t              tail;       // The tail of frame
                    size_t              size;       // The size of the frame
                    size_t              length;     // The overall length of the stream
                } frame_t;

                size_t                  nFrames;    // Number of frames
                size_t                  nChannels;  // Number of channels
                size_t                  nBufMax;    // Maximum size of buffer
                size_t                  nBufCap;    // Buffer capacity
                size_t                  nFrameCap;  // Capacity in frames

                volatile uint32_t       nFrameId;   // Current frame identifier

                frame_t                *vFrames;    // List of frames
                float                 **vChannels;  // Channel data

                uint8_t                *pData;      // Allocated channel data

            public:
                static stream_t        *create(size_t channels, size_t frames, size_t capacity);
                static void             destroy(stream_t *buf);

            public:
                /**
                 * Get the overall number of channels
                 * @return overall number of channels
                 */
                inline size_t           channels() const    { return nChannels;     }

                /**
                 * Get actual number of frames
                 * @return actual number of frames
                 */
                inline size_t           frames() const      { return nFrames;       }

                /**
                 * Get capacity of the mesh
                 * @return capacity of the mesh
                 */
                inline size_t           capacity() const    { return nBufMax;       }

                /**
                 * Get head position of the incremental frame block
                 * @return head position of the frame block
                 */
                ssize_t                 get_head(uint32_t frame) const;

                /**
                 * Get tail position of the incremental frame block
                 * @return tail position of the frame block
                 */
                ssize_t                 get_tail(uint32_t frame) const;

                /**
                 * Get size of the incremental frame block
                 * @return size of the frame block
                 */
                ssize_t                 get_frame_size(uint32_t frame) const;

                /**
                 * Get start position of the stream (including previous frames)
                 * @return start the start position of the frame
                 */
                ssize_t                 get_position(uint32_t frame) const;

                /**
                 * Get the whole length of the stream data starting with the specified frame
                 * @param frame frame identifier
                 * @return the length of the whole frame
                 */
                ssize_t                 get_length(uint32_t frame) const;

                /**
                 * Get the identifier of head frame
                 * @return identifier of head frame
                 */

                inline uint32_t         frame_id() const        { return nFrameId;      }
                /**
                 * Begin write of frame data
                 * @param size the required size of frame
                 * @return the actual size of allocated frame
                 */
                size_t                  add_frame(size_t size);

                /**
                 * Write data to the currently allocated frame
                 * @param channel channel to write data
                 * @param data source buffer to write
                 * @param off the offset inside the frame
                 * @param count number of elements to write
                 * @return number of elements written or negative error code
                 */
                ssize_t                 write_frame(size_t channel, const float *data, size_t off, size_t count);

                /**
                 * Get data for the currently allocated frame. Frame can be split into two parts because of using
                 * ring buffer, so the returned number of elements should be considered to perform right frame
                 * data update.
                 *
                 * @param channel channel number
                 * @param off the offset inside the frame
                 * @param count the pointer to store available number of elements for frame
                 * @return pointer to the frame data or NULL
                 */
                float                  *frame_data(size_t channel, size_t off, size_t *count);

                /**
                 * Read frame data
                 * @param frame frame identifier
                 * @param channel channel number
                 * @param data pointer to store the value
                 * @param off offset from the beginning of the frame
                 * @param count number of elements to read
                 * @return number of elements written or negative error code
                 */
                ssize_t                 read_frame(uint32_t frame_id, size_t channel, float *data, size_t off, size_t count);

                /**
                 * Read the whole stream according to the information stored inside of the last frame
                 * @param channel channel number
                 * @param data destination buffer
                 * @param off offset relative to the beginning of the whole frame
                 * @param count number of elements to read
                 * @return number of elements read or negative error code
                 */
                ssize_t                 read(size_t channel, float *data, size_t off, size_t count);

                /**
                 * Commit the new frame to the list of frames
                 * @return true if frame has been committed
                 */
                bool                    commit_frame();

                /**
                 * Sync state with another stream
                 * @param src stream to perform the sync
                 * @return status of operation
                 */
                bool                    sync(const stream_t *src);

                /**
                 * Clear the stream and set current frame
                 * @param current current frame
                 */
                void                    clear(uint32_t current);

                /**
                 * Clear stream and increment current frame
                 */
                void                    clear();
        } stream_t;

        /**
         * This interface describes frame buffer. All data is stored as a single rolling frame.
         * The frame consists of M data rows, each row contains N floating-point numbers.
         * While frame buffer is changing, new rows become appended to the frame buffer. Number
         * of appended/modified rows is stored in additional counter to allow the UI apply
         * changes incrementally.
         */
        typedef struct frame_buffer_t
        {
            protected:
                size_t              nRows;              // Number of rows
                size_t              nCols;              // Number of columns
                uint32_t            nCapacity;          // Capacity (power of 2)
                mutable uint32_t    nRowID;             // Unique row identifier
                float              *vData;              // Aligned row data
                uint8_t            *pData;              // Allocated row data

            public:
                static frame_buffer_t  *create(size_t rows, size_t cols);
                static void             destroy(frame_buffer_t *buf);

                status_t                init(size_t rows, size_t cols);
                void                    destroy();

            public:
                /**
                 * Return the actual data of the requested row
                 * @param dst destination buffer to store result
                 * @param row_id row number
                 */
                void read_row(float *dst, size_t row_id) const;

                /**
                 * Get pointer to row data of the corresponding row identifier
                 * @param row_id unique row identifier
                 * @return pointer to row data
                 */
                float *get_row(size_t row_id) const;

                /**
                 * Get pointer to row data of the current row identifier
                 * @param row_id unique row identifier
                 * @return pointer to data of the next row
                 */
                float *next_row() const;

                /**
                 * Return actual number of rows
                 * @return actual number of rows
                 */
                inline size_t rows() const { return nRows; }

                /**
                 * Get number of next row identifier
                 * @return next row identifier
                 */
                inline uint32_t next_rowid() const { return atomic_load(&nRowID); }

                /**
                 * Return actual number of columns
                 * @return actual number of columns
                 */
                inline size_t cols() const { return nCols; }

                /**
                 * Clear the buffer contents, set number of changes equal to buffer rows
                 */
                void clear();

                /**
                 * Seek to the specified row
                 * @param row_id unique row identifier
                 */
                void seek(uint32_t row_id);

                /** Append the new row to the beginning of frame buffer and increment current row number
                 * @param row row data contents
                 */
                void write_row(const float *row);

                /** Overwrite the row of frame buffer
                 * @param row row data contents
                 */
                void write_row(uint32_t row_id, const float *row);

                /**
                 * Just increment row counter to commit row data
                 */
                void write_row();

                /**
                 * Synchronize data to the other frame buffer
                 * @param fb frame buffer object
                 * @return true if changes from other frame buffer have been applied
                 */
                bool sync(const frame_buffer_t *fb);

        } frame_buffer_t;

        // Midi port structure
        typedef struct midi_t
        {
            size_t          nEvents;                    // Number of events in the queue
            midi::event_t   vEvents[MIDI_EVENTS_MAX];   // List of events in the queue

            /**
             * Add event to the queue
             * @param me pointer to event to add
             * @return true if event was added
             */
            inline bool push(const midi::event_t *me)
            {
                if (nEvents >= MIDI_EVENTS_MAX)
                    return false;
                vEvents[nEvents++]      = *me;
                return true;
            }

            /**
             * Add event to the queue
             * @param me event to add
             * @return true if event was added
             */
            inline bool push(const midi::event_t &me) { return push(&me); }

            /**
             * Select all events from the source queue that match specified timestamp range [start, end) and put to this queue,
             * subtract timestamp value by start from original events.
             * @note For proper work of this function the input queue should be sorted.
             *
             * @param src source queue to select data
             * @param start the start of the timestamp range
             * @param end the end of the timestamp range
             * @return true if all events have been added
             */
            bool push_slice(const midi_t *src, uint32_t start, uint32_t end);

            /**
             * Append all events from the source queue without any modifications
             * @param src source queue
             * @return true if all events have been added
             */
            bool push_all(const midi_t *src);

            /**
             * Append all events from the source queue without any modifications
             * @param src source queue
             * @return true if all events have been added
             */
            inline bool push_all(const midi_t &src) { return push_all(&src); }

            /**
             * Push all events from the source queue and add the specified offset to their timestamps
             * @param src source queue to select events
             * @param offset the offset to add to the timestamp of each event
             * @return true if all events have been added
             */
            bool push_all_shifted(const midi_t *src, uint32_t offset);

            /**
             * Copy the contents of the queue
             * @param src the contents of the queue to copy
             */
            inline void copy_from(const midi_t *src)
            {
                nEvents     = src->nEvents;
                if (nEvents > 0)
                    ::memcpy(vEvents, src->vEvents, nEvents * sizeof(midi::event_t));
            }

            /**
             * Clear destination queue and copy all contents to it
             * @param dst destination queue to copy all contents
             */
            inline void copy_to(midi_t *dst) const
            {
                dst->copy_from(this);
            }

            /**
             * Clear the queue
             */
            inline void clear()
            {
                nEvents     = 0;
            }

            /**
             * Perform sort of events stored in the queue according to their timestamps
             */
            void sort();
        } midi_t;

        // Path port structure
        typedef struct path_t
        {
            /**
             * Virtual destructor
             */
            virtual ~path_t();

            /**
             * Initialize path
             */
            virtual void init();

            /**
             * Get actual path (UTF-8 string)
             *
             * @return actual path
             */
            virtual const char *path() const;

            /**
             * Get current flags
             * @return current flags
             */
            virtual size_t flags() const;

            /**
             * Check if there is pending request
             *
             * @return true if there is a pending state-change request
             */
            virtual bool pending();

            /**
             * Accept the pending request for path change,
             * the port of the path will not trigger as changed
             * until commit() is called
             */
            virtual void accept();

            /**
             * Check if there is accepted request
             *
             * @return true if there is accepted request
             */
            virtual bool accepted();

            /**
             * The state change request was processed,
             * the port is ready to receive new events,
             * this method SHOULD be called ONLY AFTER
             * we don't need the value stored in this primitive
             */
            virtual void commit();
        } path_t;

        typedef struct string_t
        {
            char               *sData;              // Actual value available to host
            char               *sPending;           // Pending value
            uint32_t            nCapacity;          // Capacity
            uint32_t            nLock;              // Write lock + flags
            uint32_t            nSerial;            // Current serial version
            uint32_t            nRequest;           // Requested version

            /**
             * Submit string contents. If string length is larger than allowed capacity, it is truncated.
             * This method introduces atomic locks and should never be called from real-time thread.
             * @param str UTF-8 string to submit
             * @param state indicates that value has been restored from state
             * @return serial number associated with this change
             */
            uint32_t            submit(const char *str, bool state);

            /**
             * Submit string contents. If string length is larger than allowed capacity, it is truncated.
             * This method introduces atomic locks and should never be called from real-time thread.
             * @param buffer UTF-8 string to submit
             * @param size size of data in bytes
             * @param state indicates that value has been restored from state
             * @return serial number associated with this change
             */
            uint32_t            submit(const void *buffer, size_t size, bool state);

            /**
             * Submit string contents. If string length is larger than allowed capacity, it is truncated.
             * This method introduces atomic locks and should never be called from real-time thread.
             * @param str string to submit
             * @param state indicates that value has been restored from state
             * @return serial number associated with this change
             */
            uint32_t            submit(const LSPString *str, bool state);

            /**
             * Set string contents. If string length is larger than allowed capacity, it is truncated.
             * This method does the same to submit() but without locking.
             * @param str UTF-8 string to submit
             * @param state indicates that value has been restored from state
             * @return serial number associated with this change
             */
            uint32_t            set(const char *str, bool state);

            /**
             * Set string contents. If string length is larger than allowed capacity, it is truncated.
             * This method does the same to submit() but without locking.
             * @param buffer UTF-8 string to submit
             * @param size size of data in bytes
             * @param state indicates that value has been restored from state
             * @return serial number associated with this change
             */
            uint32_t            set(const void *buffer, size_t size, bool state);

            /**
             * Read current contents of the string to passed buffer if serial value differs to the passed one,
             * store new serial value into the passed pointer.
             * This method introduces atomic locks and should never be called from real-time thread.
             * @param serial pointer to the strings's serial number the requestor holds
             * @param dst destination buffer to store the string
             * @param size size of destination buffer in bytes
             * @return true if passed serial number differed to the string's serial number and
             * destination buffer was filled with data
             */
            bool                fetch(uint32_t *serial, char *dst, size_t size);

            /**
             * Synchronize state. This method is designed to be called from real-time thread to commit
             * pending state change of the string and return update status.
             * @return true if value of the string has been updated
             */
            bool                sync();

            /**
             * Check that string has been restored from plugin's state
             * @return true if string has been restored from plugin's state
             */
            bool                is_state() const;

            /**
             * Maximum number of bytes that can be stored in this string
             * @return maximum number of bytes (without trailing zero)
             */
            size_t              max_bytes() const;

            /**
             * return actual serial number
             * @return actual serial number
             */
            uint32_t            serial() const;

            /**
             * Increment serial number as internal value has been changed
             */
            void                touch();

            /**
             * Allocate string parameter
             * @param max_length maximum string length
             * @return pointer to allocated string
             */
            static string_t    *allocate(size_t max_length);

            /**
             * Destroy string parameter
             * @param str string parameter to destroy
             */
            static void         destroy(string_t *str);
        } string_t;

        // Position port structure
        typedef struct position_t
        {
            /** Current sample rate in Hz
             *
             */
            float           sampleRate;

            /** The rate of the progress of time as a fraction of normal speed.
             * For example, a rate of 0.0 is stopped, 1.0 is rolling at normal
             * speed, 0.5 is rolling at half speed, -1.0 is reverse, and so on.
             */
            double          speed;

            /** Frame number
             *
             */
            uint64_t        frame;

            /** Time signature numerator (e.g. 3 for 3/4)
             *
             */
            double          numerator;

            /** Time signature denominator (e.g. 4 for 3/4)
             *
             */
            double          denominator;

            /** Current tempo in beats per minute
             *
             */
            double          beatsPerMinute;

            /**
             * The tempo increment/decrement on each additional sample
             */
            double          beatsPerMinuteChange;

            /** Current tick within the bar
             *
             */
            double          tick;

            /** Number of ticks per beat
             *
             */
            double          ticksPerBeat;

            static void init(position_t *pos);
        } position_t;


        /**
         * Copy UTF-8 encoded string to destination string, limit number of characters to the specified.
         * The buffer should be of enough size to contain the additional zero-terminating character.
         * @param dst destination string
         * @param dst_max maximum number of UTF-8 characters (excluding trailing zero)
         * @param src source string
         */
        void utf8_strncpy(char *dst, size_t dst_max, const char *src);

        /**
         * Copy UTF-8 encoded string to destination string, limit number of characters to the specified.
         * The buffer should be of enough size to contain the additional zero-terminating character.
         * @param dst destination string
         * @param dst_max maximum number of UTF-8 characters (excluding trailing zero)
         * @param src source buffer
         * @param size size of source buffer
         */
        void utf8_strncpy(char *dst, size_t dst_max, const void *src, size_t size);

    } /* namespace plug */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_PLUG_DATA_H_ */
