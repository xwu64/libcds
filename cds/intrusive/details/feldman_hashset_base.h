//$$CDS-header$$

#ifndef CDSLIB_INTRUSIVE_DETAILS_FELDMAN_HASHSET_BASE_H
#define CDSLIB_INTRUSIVE_DETAILS_FELDMAN_HASHSET_BASE_H

#include <memory.h> // memcmp, memcpy
#include <type_traits>

#include <cds/intrusive/details/base.h>
#include <cds/opt/compare.h>
#include <cds/algo/atomic.h>
#include <cds/algo/split_bitstring.h>
#include <cds/details/marked_ptr.h>
#include <cds/urcu/options.h>

namespace cds { namespace intrusive {

    /// FeldmanHashSet related definitions
    /** @ingroup cds_intrusive_helper
    */
    namespace feldman_hashset {
        /// Hash accessor option
        /**
            @copydetails traits::hash_accessor
        */
        template <typename Accessor>
        struct hash_accessor {
            //@cond
            template <typename Base> struct pack: public Base
            {
                typedef Accessor hash_accessor;
            };
            //@endcond
        };

        /// \p FeldmanHashSet internal statistics
        template <typename EventCounter = cds::atomicity::event_counter>
        struct stat {
            typedef EventCounter event_counter ; ///< Event counter type

            event_counter   m_nInsertSuccess;   ///< Number of success \p insert() operations
            event_counter   m_nInsertFailed;    ///< Number of failed \p insert() operations
            event_counter   m_nInsertRetry;     ///< Number of attempts to insert new item
            event_counter   m_nUpdateNew;       ///< Number of new item inserted for \p update()
            event_counter   m_nUpdateExisting;  ///< Number of existing item updates
            event_counter   m_nUpdateFailed;    ///< Number of failed \p update() call
            event_counter   m_nUpdateRetry;     ///< Number of attempts to update the item
            event_counter   m_nEraseSuccess;    ///< Number of successful \p erase(), \p unlink(), \p extract() operations
            event_counter   m_nEraseFailed;     ///< Number of failed \p erase(), \p unlink(), \p extract() operations
            event_counter   m_nEraseRetry;      ///< Number of attempts to \p erase() an item
            event_counter   m_nFindSuccess;     ///< Number of successful \p find() and \p get() operations
            event_counter   m_nFindFailed;      ///< Number of failed \p find() and \p get() operations

            event_counter   m_nExpandNodeSuccess; ///< Number of succeeded attempts converting data node to array node
            event_counter   m_nExpandNodeFailed;  ///< Number of failed attempts converting data node to array node
            event_counter   m_nSlotChanged;     ///< Number of array node slot changing by other thread during an operation
            event_counter   m_nSlotConverting;  ///< Number of events when we encounter a slot while it is converting to array node

            event_counter   m_nArrayNodeCount;  ///< Number of array nodes
            event_counter   m_nHeight;          ///< Current height of the tree

            //@cond
            void onInsertSuccess()              { ++m_nInsertSuccess;       }
            void onInsertFailed()               { ++m_nInsertFailed;        }
            void onInsertRetry()                { ++m_nInsertRetry;         }
            void onUpdateNew()                  { ++m_nUpdateNew;           }
            void onUpdateExisting()             { ++m_nUpdateExisting;      }
            void onUpdateFailed()               { ++m_nUpdateFailed;        }
            void onUpdateRetry()                { ++m_nUpdateRetry;         }
            void onEraseSuccess()               { ++m_nEraseSuccess;        }
            void onEraseFailed()                { ++m_nEraseFailed;         }
            void onEraseRetry()                 { ++m_nEraseRetry;          }
            void onFindSuccess()                { ++m_nFindSuccess;         }
            void onFindFailed()                 { ++m_nFindFailed;          }

            void onExpandNodeSuccess()          { ++m_nExpandNodeSuccess;   }
            void onExpandNodeFailed()           { ++m_nExpandNodeFailed;    }
            void onSlotChanged()                { ++m_nSlotChanged;         }
            void onSlotConverting()             { ++m_nSlotConverting;      }
            void onArrayNodeCreated()           { ++m_nArrayNodeCount;      }
            void height( size_t h )             { if (m_nHeight < h ) m_nHeight = h; }
            //@endcond
        };

        /// \p FeldmanHashSet empty internal statistics
        struct empty_stat {
            //@cond
            void onInsertSuccess()              const {}
            void onInsertFailed()               const {}
            void onInsertRetry()                const {}
            void onUpdateNew()                  const {}
            void onUpdateExisting()             const {}
            void onUpdateFailed()               const {}
            void onUpdateRetry()                const {}
            void onEraseSuccess()               const {}
            void onEraseFailed()                const {}
            void onEraseRetry()                 const {}
            void onFindSuccess()                const {}
            void onFindFailed()                 const {}

            void onExpandNodeSuccess()          const {}
            void onExpandNodeFailed()           const {}
            void onSlotChanged()                const {}
            void onSlotConverting()             const {}
            void onArrayNodeCreated()           const {}
            void height(size_t)                 const {}
            //@endcond
        };

        /// \p FeldmanHashSet traits
        struct traits
        {
            /// Mandatory functor to get hash value from data node
            /**
                It is most-important feature of \p FeldmanHashSet.
                That functor must return a reference to fixed-sized hash value of data node.
                The return value of that functor specifies the type of hash value.

                Example:
                \code
                typedef uint8_t hash_type[32]; // 256-bit hash type
                struct foo {
                    hash_type  hash; // 256-bit hash value
                    // ... other fields
                };

                // Hash accessor
                struct foo_hash_accessor {
                    hash_type const& operator()( foo const& d ) const
                    {
                        return d.hash;
                    }
                };
                \endcode
            */
            typedef cds::opt::none hash_accessor;

            /// Disposer for removing data nodes
            typedef cds::intrusive::opt::v::empty_disposer disposer;

            /// Hash comparing functor
            /**
                No default functor is provided.
                If the option is not specified, the \p less option is used.
            */
            typedef cds::opt::none compare;

            /// Specifies binary predicate used for hash compare.
            /**
                If \p %less and \p %compare are not specified, \p memcmp() -like @ref bitwise_compare "bit-wise hash comparator" is used
                because the hash value is treated as fixed-sized bit-string.
            */
            typedef cds::opt::none less;

            /// Item counter
            /**
                The item counting is an important part of \p FeldmanHashSet algorithm:
                the \p empty() member function depends on correct item counting.
                Therefore, \p atomicity::empty_item_counter is not allowed as a type of the option.

                Default is \p atomicity::item_counter.
            */
            typedef cds::atomicity::item_counter item_counter;

            /// Array node allocator
            /**
                Allocator for array nodes. That allocator is used for creating \p headNode and \p arrayNode when the set grows.
                Default is \ref CDS_DEFAULT_ALLOCATOR
            */
            typedef CDS_DEFAULT_ALLOCATOR node_allocator;

            /// C++ memory ordering model
            /**
                Can be \p opt::v::relaxed_ordering (relaxed memory model, the default)
                or \p opt::v::sequential_consistent (sequentially consisnent memory model).
            */
            typedef cds::opt::v::relaxed_ordering memory_model;

            /// Back-off strategy
            typedef cds::backoff::Default back_off;

            /// Internal statistics
            /**
                By default, internal statistics is disabled (\p feldman_hashset::empty_stat).
                Use \p feldman_hashset::stat to enable it.
            */
            typedef empty_stat stat;

            /// RCU deadlock checking policy (only for \ref cds_intrusive_FeldmanHashSet_rcu "RCU-based FeldmanHashSet")
            /**
                List of available policy see \p opt::rcu_check_deadlock
            */
            typedef cds::opt::v::rcu_throw_deadlock rcu_check_deadlock;
        };

        /// Metafunction converting option list to \p feldman_hashset::traits
        /**
            Supported \p Options are:
            - \p feldman_hashset::hash_accessor - mandatory option, hash accessor functor.
                @copydetails traits::hash_accessor
            - \p opt::node_allocator - array node allocator.
                @copydetails traits::node_allocator
            - \p opt::compare - hash comparison functor. No default functor is provided.
                If the option is not specified, the \p opt::less is used.
            - \p opt::less - specifies binary predicate used for hash comparison.
                If the option is not specified, \p memcmp() -like bit-wise hash comparator is used
                because the hash value is treated as fixed-sized bit-string.
            - \p opt::back_off - back-off strategy used. If the option is not specified, the \p cds::backoff::Default is used.
            - \p opt::disposer - the functor used for disposing removed data node. Default is \p opt::v::empty_disposer. Due the nature
                of GC schema the disposer may be called asynchronously.
            - \p opt::item_counter - the type of item counting feature.
                 The item counting is an important part of \p FeldmanHashSet algorithm:
                 the \p empty() member function depends on correct item counting.
                 Therefore, \p atomicity::empty_item_counter is not allowed as a type of the option.
                 Default is \p atomicity::item_counter.
            - \p opt::memory_model - C++ memory ordering model. Can be \p opt::v::relaxed_ordering (relaxed memory model, the default)
                or \p opt::v::sequential_consistent (sequentially consisnent memory model).
            - \p opt::stat - internal statistics. By default, it is disabled (\p feldman_hashset::empty_stat).
                To enable it use \p feldman_hashset::stat
            - \p opt::rcu_check_deadlock - a deadlock checking policy for \ref cds_intrusive_FeldmanHashSet_rcu "RCU-based FeldmanHashSet"
                Default is \p opt::v::rcu_throw_deadlock
        */
        template <typename... Options>
        struct make_traits
        {
#   ifdef CDS_DOXYGEN_INVOKED
            typedef implementation_defined type ;   ///< Metafunction result
#   else
            typedef typename cds::opt::make_options<
                typename cds::opt::find_type_traits< traits, Options... >::type
                ,Options...
            >::type   type;
#   endif
        };

        /// Bit-wise memcmp-based comparator for hash value \p T
        template <typename T>
        struct bitwise_compare
        {
            /// Compares \p lhs and \p rhs
            /**
                Returns:
                - <tt> < 0</tt> if <tt>lhs < rhs</tt>
                - <tt>0</tt> if <tt>lhs == rhs</tt>
                - <tt> > 0</tt> if <tt>lhs > rhs</tt>
            */
            int operator()( T const& lhs, T const& rhs ) const
            {
                return memcmp( &lhs, &rhs, sizeof(T));
            }
        };

        /// One-level statistics, see \p FeldmanHashSet::get_level_statistics
        struct level_statistics
        {
            size_t array_node_count;    ///< Count of array node at the level
            size_t node_capacity;       ///< Array capacity

            size_t data_cell_count;     ///< The number of data cells in all array node at this level
            size_t array_cell_count;    ///< The number of array cells in all array node at this level
            size_t empty_cell_count;    ///< The number of empty cells in all array node at this level

            //@cond
            level_statistics()
                : array_node_count(0)
                , data_cell_count(0)
                , array_cell_count(0)
                , empty_cell_count(0)
            {}
            //@endcond
        };

        //@cond
        namespace details {
            template <typename HashType, typename UInt = size_t >
            using hash_splitter = cds::algo::split_bitstring< HashType, UInt >;

            struct metrics {
                size_t  head_node_size;     // power-of-two
                size_t  head_node_size_log; // log2( head_node_size )
                size_t  array_node_size;    // power-of-two
                size_t  array_node_size_log;// log2( array_node_size )

                static metrics make(size_t head_bits, size_t array_bits, size_t hash_size )
                {
                    size_t const hash_bits = hash_size * 8;

                    if (array_bits < 2)
                        array_bits = 2;
                    if (head_bits < 4)
                        head_bits = 4;
                    if (head_bits > hash_bits)
                        head_bits = hash_bits;
                    if ((hash_bits - head_bits) % array_bits != 0)
                        head_bits += (hash_bits - head_bits) % array_bits;

                    assert((hash_bits - head_bits) % array_bits == 0);

                    metrics m;
                    m.head_node_size_log = head_bits;
                    m.head_node_size = size_t(1) << head_bits;
                    m.array_node_size_log = array_bits;
                    m.array_node_size = size_t(1) << array_bits;
                    return m;
                }
            };

        } // namespace details
        //@endcond
    } // namespace feldman_hashset

    //@cond
    // Forward declaration
    template < class GC, typename T, class Traits = feldman_hashset::traits >
    class FeldmanHashSet;
    //@endcond

}} // namespace cds::intrusive

#endif // #ifndef CDSLIB_INTRUSIVE_DETAILS_FELDMAN_HASHSET_BASE_H
