/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_KVTSTORAGE_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_KVTSTORAGE_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/darray.h>

namespace lsp
{
    namespace core
    {
        enum kvt_param_type_t
        {
            KVT_ANY,                // Any type

            KVT_INT32,
            KVT_UINT32,
            KVT_INT64,
            KVT_UINT64,
            KVT_FLOAT32,            // 32-bit floating-point value
            KVT_FLOAT64,            // 64-bit floating-point value
            KVT_STRING,             // UTF-8 string
            KVT_BLOB                // Binary data
        };

        enum kvt_flags_t
        {
            KVT_RX              = 1 << 0,       // Transfer from UI to DSP
            KVT_TX              = 1 << 1,       // Transfer from DSP to UI
            KVT_KEEP            = 1 << 2,       // Keep the previous value of parameter if it exists
            KVT_DELEGATE        = 1 << 3,       // Delegate the control over parameter's data to the storage
            KVT_PRIVATE         = 1 << 4,       // Private option (do not transfer)
            KVT_TRANSIENT       = 1 << 5,       // Transient option (do not serialize)

            // Special constants to not to be confused with KVT_RX and KVT_TX abbreviations
            KVT_TO_UI           = KVT_TX,
            KVT_TO_DSP          = KVT_RX
        };

        typedef struct kvt_blob_t
        {
            const char    *ctype;          // Content type
            const void    *data;           // Pointer to data
            size_t         size;           // Size of blob
        } kvt_blob_t;

        typedef struct kvt_param_t
        {
            kvt_param_type_t type;
            union
            {
                int32_t         i32;
                uint32_t        u32;
                int64_t         i64;
                uint64_t        u64;
                float           f32;
                double          f64;
                const char     *str;
                kvt_blob_t      blob;
            };
        } kvt_param_t;

        class KVTStorage;

        class KVTListener
        {
            public:
                explicit KVTListener();
                virtual ~KVTListener();

            public:
                /**
                 * Listener has been bound to the storage
                 * @param storage storage
                 */
                virtual void attached(KVTStorage *storage);

                /**
                 * Listener has been unbound from the storage
                 * @param storage storage
                 */
                virtual void detached(KVTStorage *storage);

                /**
                 * Triggers event when parameter has been created
                 * @param storage KVT storage that triggered the event
                 * @param id the full unique identifier of parameter
                 * @param param parameter
                 * @param pending pending flags (KVT_TX or KVT_RX)
                 */
                virtual void created(KVTStorage *storage, const char *id, const kvt_param_t *param, size_t pending);

                /**
                 * Triggers event when parameter has been rejected (the put() method
                 * tried to replace the existing parameter)
                 * @param storage KVT storage that triggered the event
                 * @param id the full unique identifier of parameter
                 * @param rej the rejected parameter
                 * @param curr current value parameter
                 * @param pending pending flags (KVT_TX or KVT_RX)
                 */
                virtual void rejected(KVTStorage *storage, const char *id, const kvt_param_t *rej, const kvt_param_t *curr, size_t pending);

                /**
                 * Triggers event when parameter has been changed
                 * @param storage KVT storage that triggered the event
                 * @param id the full unique identifier of parameter
                 * @param oval old value of parameter
                 * @param nval new value of parameter
                 * @param pending pending flags (KVT_TX or KVT_RX)
                 */
                virtual void changed(KVTStorage *storage, const char *id, const kvt_param_t *oval, const kvt_param_t *nval, size_t pending);

                /**
                 * Triggers event when parameter has been removed
                 * @param storage KVT storage that triggered the event
                 * @param id the full unique identifier of parameter
                 * @param param the value of removed parameter
                 * @param pending pending flags (KVT_TX or KVT_RX)
                 */
                virtual void removed(KVTStorage *storage, const char *id, const kvt_param_t *param, size_t pending);

                /**
                 * The parameter has been accessed for reading
                 * @param storage KVT storage that triggered the event
                 * @param id parameter identifier
                 * @param param parameter
                 * @param pending pending flags (KVT_TX or KVT_RX)
                 */
                virtual void access(KVTStorage *storage, const char *id, const kvt_param_t *param, size_t pending);

                /**
                 * The parameter has been committed (it's modification flag has been reset)
                 * @param storage KVT storage that triggered the event
                 * @param id parameter identifier
                 * @param pending pending flags (KVT_TX or KVT_RX)
                 * @param param parameter parameter value
                 */
                virtual void commit(KVTStorage *storage, const char *id, const kvt_param_t *param, size_t pending);

                /**
                 * The parameter has been accessed for reading/removal
                 * @param storage KVT storage that triggered the event
                 * @param id parameter identifier
                 * @param param parameter
                 */
                virtual void missed(KVTStorage *storage, const char *id);
        };

        class KVTIterator;

        /**
         * Key-value tree storage
         */
        class KVTStorage
        {
            private:
                friend class KVTIterator;

                enum iterator_mode_t
                {
                    IT_INVALID,
                    IT_TX_PENDING,
                    IT_RX_PENDING,
                    IT_ALL,
                    IT_BRANCH,
                    IT_RECURSIVE,
                    IT_EOF
                };

            protected:
                struct kvt_node_t;

                typedef struct kvt_link_t {
                    kvt_link_t         *prev;
                    kvt_link_t         *next;
                    kvt_node_t         *node;
                } kvt_link_t;

                typedef struct kvt_gcparam_t : public kvt_param_t {
                    size_t              flags;
                    kvt_gcparam_t      *next;
                } kvt_gcparam_t;

                typedef struct kvt_node_t
                {
                    char               *id;             // Unique node identifier
                    size_t              idlen;          // Length of the ID string
                    kvt_node_t         *parent;         // Parent node
                    ssize_t             refs;           // Number of references
                    kvt_gcparam_t      *param;          // Currently used parameter

                    size_t              pending;         // Transmission flags
                    kvt_link_t          gc;             // Link to the removed list
                    kvt_link_t          rx;             // Link to the Rx modified list
                    kvt_link_t          tx;             // Link to the Tx modified list

                    kvt_node_t        **children;       // Children
                    size_t              nchildren;      // Number of children
                    size_t              capacity;       // Capacity in children
                } kvt_node_t;

            protected:
                lltl::parray<KVTListener>   vListeners;

                kvt_link_t              sValid;
                kvt_link_t              sTx;
                kvt_link_t              sRx;
                kvt_link_t              sGarbage;
                char                    cSeparator;
                kvt_gcparam_t          *pTrash;
                KVTIterator            *pIterators;
                kvt_node_t              sRoot;
                size_t                  nValues;
                size_t                  nNodes;
                size_t                  nTxPending;
                size_t                  nRxPending;

            protected:
                inline void             notify_created(const char *id, const kvt_param_t *param, size_t pending);
                inline void             notify_rejected(const char *id, const kvt_param_t *rej, const kvt_param_t *curr, size_t pending);
                inline void             notify_changed(const char *id, const kvt_param_t *oval, const kvt_param_t *nval, size_t flags);
                inline void             notify_removed(const char *id, const kvt_param_t *param, size_t pending);
                inline void             notify_access(const char *id, const kvt_param_t *param, size_t pending);
                inline void             notify_commit(const char *id, const kvt_param_t *param, size_t pending);
                inline void             notify_missed(const char *id);

            protected:
                inline static void      link_list(kvt_link_t *root, kvt_link_t *item);
                inline static void      unlink_list(kvt_link_t *item);

                size_t                  set_pending_state(kvt_node_t *node, size_t flags);
                kvt_node_t             *reference_up(kvt_node_t *node);
                kvt_node_t             *reference_down(kvt_node_t *node);

                char                   *build_path(char **path, size_t *capacity, const kvt_node_t *node);

                void                    destroy_parameter(kvt_gcparam_t *p);
                status_t                commit_parameter(const char *path, kvt_node_t *node, const kvt_param_t *value, size_t flags);
                kvt_gcparam_t          *copy_parameter(const kvt_param_t *src, size_t flags);

                inline void             init_node(kvt_node_t *node, const char *name, size_t len);
                kvt_node_t             *allocate_node(const char *name, size_t len);
                kvt_node_t             *create_node(kvt_node_t *base, const char *name, size_t len);
                void                    destroy_node(kvt_node_t *node);
                kvt_node_t             *get_node(kvt_node_t *base, const char *name, size_t len);
                status_t                walk_node(kvt_node_t **out, const char *name);

                status_t                do_remove_node(const char *name, kvt_node_t *node, const kvt_param_t **value, kvt_param_type_t type);
                status_t                do_touch(const char *name, kvt_node_t *node, size_t flags);
                status_t                do_commit(const char *name, kvt_node_t *node, size_t flags);
                status_t                do_remove_branch(const char *name, kvt_node_t *node);

                static inline bool      validate_type(size_t type) { return (type >= KVT_INT32) && (type <= KVT_BLOB); }

            public:
                explicit KVTStorage(char separator = '/');
                KVTStorage(const KVTStorage &) = delete;
                KVTStorage(KVTStorage &&) = delete;
                ~KVTStorage();

                KVTStorage & operator = (const KVTStorage &) = delete;
                KVTStorage & operator = (KVTStorage &&) = delete;

                void                    destroy();
                status_t                clear();

            public:
                /**
                 * Bind listener to the storage
                 * @param listener listener to bind
                 * @return status of operation
                 */
                status_t    bind(KVTListener *listener);

                /**
                 * Unbind listener from storage
                 * @param listener listener to unbind
                 * @return status of operation
                 */
                status_t    unbind(KVTListener *listener);

                /**
                 * Check that listener is bound
                 * @param listener listener to check
                 * @return status of operation
                 */
                status_t    is_bound(KVTListener *listener);

                /**
                 * Unbind all listeners
                 * @return status of operation
                 */
                status_t    unbind_all();

            public:
                inline  size_t nodes() const        { return nNodes;        }
                inline  size_t values() const       { return nValues;       }
                inline  size_t tx_pending() const   { return nTxPending;    }
                inline  size_t rx_pending() const   { return nRxPending;    }
                size_t         listeners() const;

            public:
                /**
                 * Put parameter to the storage
                 * @param name parameter name
                 * @param value parameter value
                 * @param flags operation flags @see kvt_flags_t
                 * @return status of operation:
                 *          STATUS_OK               if parameter has been stored
                 *          STATUS_ALREADY_EXISTS   if parameter has been rejected (only if KVT_KEEP is set)
                 *          other error             otherwise
                 */
                status_t    put(const char *name, const kvt_param_t *value, size_t flags = 0);
                status_t    put(const char *name, uint32_t value, size_t flags = 0);
                status_t    put(const char *name, int32_t value, size_t flags = 0);
                status_t    put(const char *name, uint64_t value, size_t flags = 0);
                status_t    put(const char *name, int64_t value, size_t flags = 0);
                status_t    put(const char *name, float value, size_t flags = 0);
                status_t    put(const char *name, double value, size_t flags = 0);
                status_t    put(const char *name, const char *value, size_t flags = 0);
                status_t    put(const char *name, const kvt_blob_t *value, size_t flags = 0);
                status_t    put(const char *name, size_t size, const char *type, const void *value, size_t flags = 0);

                /**
                 * Fetch parameter from the storage
                 * @param name parameter name
                 * @param value pointer to store the pointer to the parameter
                 * @param cast cast the value to the type if possible
                 * @return status of operation:
                 *          STATUS_OK           if parameter has been successfully read
                 *          STATUS_NOT_FOUND    if parameter has been not found
                 *          STATUS_BAD_TYPE     if parameter has incompatible type
                 */
                status_t    get(const char *name, const kvt_param_t **value, kvt_param_type_t type = KVT_ANY);
                status_t    get(const char *name, uint32_t *value);
                status_t    get(const char *name, int32_t *value);
                status_t    get(const char *name, uint64_t *value);
                status_t    get(const char *name, int64_t *value);
                status_t    get(const char *name, float *value);
                status_t    get(const char *name, double *value);
                status_t    get(const char *name, const char **value);
                status_t    get(const char *name, const kvt_blob_t **value);

                status_t    get_dfl(const char *name, uint32_t *value, uint32_t dfl);
                status_t    get_dfl(const char *name, int32_t *value, int32_t dfl);
                status_t    get_dfl(const char *name, uint64_t *value, uint64_t dfl);
                status_t    get_dfl(const char *name, int64_t *value, int64_t dfl);
                status_t    get_dfl(const char *name, float *value, float dfl);
                status_t    get_dfl(const char *name, double *value, double dfl);
                status_t    get_dfl(const char *name, const char **value, const char *dfl);

                /**
                 * Check that parameter of specified type exists
                 * @param name parameter name
                 * @param type parameter type (optional)
                 * @return true if parameter with such type exists
                 */
                bool        exists(const char *name, kvt_param_type_t type = KVT_ANY);

                /**
                 * Remove parameter from storage
                 * @param name parameter name
                 * @param value pointer to store the parameter's value
                 * @param type value type
                 * @return status of operation:
                 *          STATUS_OK           if parameter has been removed
                 *          STATUS_NOT_FOUND    if parameter has been not found
                 */
                status_t    remove(const char *name, const kvt_param_t **value = NULL, kvt_param_type_t type = KVT_ANY);
                status_t    remove(const char *name, uint32_t *value);
                status_t    remove(const char *name, int32_t *value);
                status_t    remove(const char *name, uint64_t *value);
                status_t    remove(const char *name, int64_t *value);
                status_t    remove(const char *name, float *value);
                status_t    remove(const char *name, double *value);
                status_t    remove(const char *name, const char **value);
                status_t    remove(const char *name, const kvt_blob_t **value);

                /**
                 * Set the state of node
                 * @param name node name
                 * @param modified modification flag (KVT_TX and KVT_RX)
                 * @return status of operation
                 *          STATUS_OK           if parameter has been removed
                 *          STATUS_NOT_FOUND    if parameter has been not found
                 */
                status_t    touch(const char *name, size_t flags);
                status_t    commit(const char *name, size_t flags);

                status_t    touch_all(size_t flags);
                status_t    commit_all(size_t flags);

                /**
                 * Remove the full parameter branch from storage
                 * @param name parameter name
                 * @return status of operation:
                 *          STATUS_OK           if parameter has been removed
                 *          STATUS_NOT_FOUND    if parameter has been not found
                 */
                status_t    remove_branch(const char *name);

                /**
                 * Perform garbage collection. Any of previously returned pointers to strings and
                 * blobs can become invalid
                 * @return status of operation
                 */
                status_t    gc();

            public:

                /**
                 * Enumerate the pending for transfer elements.
                 * The returned iterator will be automatically garbage-collected on gc() call.
                 *
                 * @return element iterator
                 */
                KVTIterator *enum_tx_pending();

                /**
                 * Enumerate the pending for receive elements.
                 * The returned iterator will be automatically garbage-collected on gc() call.
                 *
                 * @return element iterator
                 */
                KVTIterator *enum_rx_pending();

                /**
                 * Enumerate the whole contents of storage.
                 * The returned iterator will be automatically garbage-collected on gc() call.
                 *
                 * @return element iterator
                 */
                KVTIterator *enum_all();

                /**
                 * Enumerate the whole KVT tree branch
                 * The returned iterator will be automatically garbage-collected on gc() call.
                 *
                 * @param name branch name
                 * @param recursive enumerate sub-elements of the KVT tree branch
                 * @return element iterator
                 */
                KVTIterator *enum_branch(const char *name, bool recursive = false);
        };

        class KVTIterator
        {
            protected:
                typedef struct kvt_path_t
                {
                    KVTStorage::kvt_node_t *node;
                    size_t                  index;
                } kvt_path_t;

            protected:
                KVTStorage::kvt_node_t              sFake;
                KVTStorage::iterator_mode_t         enMode;
                KVTStorage::kvt_node_t             *pCurr;
                KVTStorage::kvt_node_t             *pNext;
                size_t                              nIndex;

                lltl::darray<kvt_path_t>            vPath;
                mutable char                       *pPath;
                mutable char                       *pData;
                mutable size_t                      nDataCap;

                KVTStorage                         *pStorage;
                KVTIterator                        *pGcNext;

                friend class KVTStorage;

            protected:
                explicit KVTIterator(KVTStorage *storage, KVTStorage::kvt_node_t *node, KVTStorage::iterator_mode_t mode);
                KVTIterator(const KVTIterator &) = delete;
                KVTIterator(KVTIterator &&) = delete;
                virtual ~KVTIterator();

                KVTIterator & operator = (const KVTIterator &) = delete;
                KVTIterator & operator = (KVTIterator &&) = delete;

            public:
                /**
                 * Iterate to the next entry
                 * @return STATUS_OK if the next entry is present, STATUS_NOT_FOUND if no record found
                 */
                status_t    next();
                bool        valid() const;
                bool        tx_pending() const;
                bool        rx_pending() const;
                size_t      pending() const;
                size_t      flags() const;
                inline bool is_transient() const    { return flags() & KVT_TRANSIENT; }
                inline bool is_private() const      { return flags() & KVT_PRIVATE; }

            public:

                bool        exists(kvt_param_type_t type = KVT_ANY) const;

                status_t    get(const kvt_param_t **value, kvt_param_type_t type = KVT_ANY);
                status_t    get(uint32_t *value);
                status_t    get(int32_t *value);
                status_t    get(uint64_t *value);
                status_t    get(int64_t *value);
                status_t    get(float *value);
                status_t    get(double *value);
                status_t    get(const char **value);
                status_t    get(const kvt_blob_t **value);

                status_t    put(const kvt_param_t *value, size_t flags = 0);
                status_t    put(uint32_t value, size_t flags = 0);
                status_t    put(int32_t value, size_t flags = 0);
                status_t    put(uint64_t value, size_t flags = 0);
                status_t    put(int64_t value, size_t flags = 0);
                status_t    put(float value, size_t flags = 0);
                status_t    put(double value, size_t flags = 0);
                status_t    put(const char *value, size_t flags = 0);
                status_t    put(const kvt_blob_t *value, size_t flags = 0);
                status_t    put(size_t size, const char *type, const void *value, size_t flags = 0);

                status_t    remove(const kvt_param_t **value = NULL, kvt_param_type_t type = KVT_ANY);
                status_t    remove(uint32_t *value);
                status_t    remove(int32_t *value);
                status_t    remove(uint64_t *value);
                status_t    remove(int64_t *value);
                status_t    remove(float *value);
                status_t    remove(double *value);
                status_t    remove(const char **value);
                status_t    remove(const kvt_blob_t **value);

                status_t    touch(size_t flags);
                status_t    commit(size_t flags);

                status_t    remove_branch();

                const char *name() const;
                const char *id() const;
        };

    #ifdef LSP_DEBUG
        void            kvt_dump_parameter(const char *fmt, const kvt_param_t *param...);
    #else
        inline void     kvt_dump_parameter(const char *fmt, const kvt_param_t *param...) {}
    #endif

        void                    kvt_init_parameter(kvt_param_t *p);
        void                    kvt_destroy_parameter(kvt_param_t *p);
        status_t                kvt_copy_parameter(kvt_param_t *dst, const kvt_param_t *src);

    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_KVTSTORAGE_H_ */
