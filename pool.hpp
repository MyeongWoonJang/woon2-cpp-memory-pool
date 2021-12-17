#ifndef _pool
#define _pool

#include <set>

class pool
{
public:
    using byte = char;

private:
    // Empty Memory Manager
    class Empty
    {
    private:
        // ===================================================================
        // Empty Memory Manager manages empty memory blocks.
        // users obtain memory by get_mem(), return memory by return_mem().
        // best_fit memory block policy is adopted.
        // memory block update occurs on every single call of get_mem() or return_mem()
        // ===================================================================

        // memory block ------------------------------------------------------

        struct mem_block
        {
            // data for memory block =========================================
            byte* start;
            size_t size;
            // ===============================================================

            mem_block( byte* const start, const size_t size )
                : start{ start }, size{ size } {}

            const bool operator==( const mem_block& rhs ) const noexcept { return start == rhs.start; }
                
            struct less_size { const bool operator()( const mem_block lhs, const mem_block rhs ) const noexcept { return lhs.size < rhs.size; } };       
            struct less_start { const bool operator()( const mem_block lhs, const mem_block rhs ) const noexcept { return lhs.start < rhs.start; } };
        };

        // data for Empty Memory Manager  ====================================
        std::set< mem_block, mem_block::less_size > size_aligned;
        std::set< mem_block, mem_block::less_start > start_aligned;
        // ===================================================================

    public:
        #include <iostream>
        void debug_print() const noexcept
        {
            std::cout << "size_aligned:\n";
            for ( const auto& item : size_aligned)
            {
                std::cout << "start: " << (int)item.start << ", size: " << item.size << '\n';
            }

            std::cout << "\n\n\n";

            std::cout << "start_aligned:\n";
            for ( const auto& item : start_aligned)
            {
                std::cout << "start: " << (int)item.start << ", size: " << item.size << '\n';
            }

            std::cout << "\n\n\n";
        }

        byte* get_mem( const size_t size )
        {
            debug_print();
            // best_fit
            auto ret_iter = size_aligned.lower_bound( mem_block{ nullptr, size } );

            // if there is no available memory for required size,
            // it's bad allocation.
            if ( ret_iter == size_aligned.end() ) throw std::bad_alloc{};

            auto ret = *ret_iter;

            // block is extracted.
            size_aligned.erase( ret_iter );
            start_aligned.erase( ret );

            // if required size is smaller than block size, rest memory is still available.
            if ( size != ret.size )
            {
                mem_block new_empty_block{ ret.start + size, ret.size - size };

                size_aligned.insert( new_empty_block );
                start_aligned.insert( new_empty_block );
            }

            debug_print();

            return ret.start;
        }

        void return_mem( byte* const exhausted, const size_t size ) noexcept
        {
            mem_block new_empty_block{ exhausted, size };

            // evaluate if there is a block to merge.

            // no empty block, no merge.
            if ( size_aligned.empty() )
            {
                size_aligned.insert(new_empty_block);
                start_aligned.insert(new_empty_block);
                return;
            }

            // check left.
            for ( auto iter = --start_aligned.lower_bound( new_empty_block ); iter != start_aligned.end(); --iter )
            {
                // if right from a memory block [left, right] is just before new empty block,
                if ( iter->start + iter->size == new_empty_block.start )
                {
                    // merge left
                    new_empty_block.start = iter->start;
                    new_empty_block.size += iter->size;

                    // after merge, there is a possibility of continuous merge.
                }
                else break;
            }

            // check right.
            for ( auto iter = start_aligned.upper_bound( new_empty_block ); iter != start_aligned.end(); ++iter )
            {
                // if left from a memory block [left, right] is just after new empty block,
                if ( new_empty_block.start + new_empty_block.size == iter->start )
                {
                    // merge right
                    new_empty_block.size += iter->size;

                    // after merge, there is a possibility of continuous merge.
                }
                else break;
            }

            std::cout << "new merged start: " << (int)new_empty_block.start << '\n';
            std::cout << "new merged size: " << new_empty_block.size << '\n';

            // extract every memory block merged.
            // memory blocks in [new_empty_block.start, exhausted) are merged-left.
            // memory blocks in (exhausted, new_empty_block.start + new_empty_block.size - 1] are merged-right.
            // so memory blocks in [new_empty_block.start, new_empty_block.start + new_empty_block.size - 1] are merged.
            auto boundary = mem_block{ new_empty_block.start + new_empty_block.size, 0 };
            for ( auto iter = start_aligned.lower_bound( new_empty_block );
                iter != start_aligned.end() && iter != start_aligned.lower_bound( boundary ); )
            {
                size_aligned.erase( *iter );
                iter = start_aligned.erase( iter );
            }

            size_aligned.insert( new_empty_block );
            start_aligned.insert( new_empty_block );
        }

        // Empty Memory Manager constructor ==================================
        Empty( byte* const mem, const size_t size )
            : size_aligned{ {mem, size} }, start_aligned{ {mem, size} } {}
        // ===================================================================
    };

    // data for pool =================================================
    byte* raw_mem;
    Empty free_mem;
    // ===============================================================

public:
    // construct Ty on gotten mem, by forwarded args( constructor args ).
    template< typename Ty, typename ... Args >
    Ty* alloc( Args&& ... args )
    {
        return new( free_mem.get_mem( sizeof( Ty ) ) ) Ty( std::forward< Args >( args )... );
    }

    // return allocated memory.
    template< typename Ty >
    void dealloc( Ty* const exhausted ) noexcept
    {
        free_mem.return_mem( reinterpret_cast< byte* >( exhausted ), sizeof( Ty ) );
    }
    
    

    // ======================================================
    // pool special member functions
    // ======================================================

    pool( const size_t mem_size )
        : raw_mem{ new byte[ mem_size ] }, free_mem{ raw_mem, mem_size }
    {
        
    }

    pool( const pool& ) = delete;
    pool& operator=( const pool& ) = delete;

    pool( pool&& other )
        : raw_mem{ other.raw_mem }, free_mem{ std::move(other.free_mem) }
    {
        other.raw_mem = nullptr;
    }

    pool& operator=( pool&& other )
    {
        if ( this != &other )
        {
            raw_mem = other.raw_mem;
            free_mem = std::move(other.free_mem);
        }

        return *this;
    }

    ~pool() {}

    // ======================================================
};

#endif // _pool