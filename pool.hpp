#ifndef _pool
#define _pool

#include <set>

class pool
{
public:
    using byte = char;

private:
    // Empty Memory Manager --------------------------------------------------
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

        // memory block end --------------------------------------------------

        // data for Empty Memory Manager  ====================================
        std::multiset< mem_block, mem_block::less_size > size_aligned;
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

        const bool cannot_insertable( byte* const mem ) const
        {
            // block intersection check

            // no block, no intersection.
            if ( start_aligned.empty() ) return false;

            // cannot insertable if mem is "in" just left block
            //     right block is not considered, right block's start is always over then mem.
            //     so mem cannot be in range [right block start, right block start + right block size).
            auto just_left = --start_aligned.lower_bound( mem_block{ mem, 0 } );
            // no left, no intersection.
            if ( just_left == start_aligned.end() ) return false;
            if ( just_left->start + just_left->size > mem ) return true;
            return false;
        }

        byte* get_mem( const size_t size )
        {
            // best_fit
            auto ret_iter = size_aligned.lower_bound( mem_block{ nullptr, size } );

            // if there is no empty memory for required size,
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

            return ret.start;
        }

        void return_mem( byte* const exhausted, const size_t size ) noexcept
        {
            std::cout << "returning " << (int)exhausted << ", size: " << size << "...\n";

            mem_block new_empty_block{ exhausted, size };

            // evaluate if there are blocks to merge.

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
                // if a memory block is just before new empty block,
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
                // if a memory block is just after new empty block,
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
            // memory blocks in range [new_empty_block.start, exhausted) are merged-left.
            // memory blocks in range (exhausted, new_empty_block.start + new_empty_block.size) are merged-right.
            // so memory blocks in range [new_empty_block.start, new_empty_block.start + new_empty_block.size) are merged.
            auto upper_limit = new_empty_block.start + new_empty_block.size;
            for ( auto iter = start_aligned.lower_bound( new_empty_block ); iter != start_aligned.end(); )
            {
                if ( iter->start >= upper_limit ) break;

                for ( auto size_iter = size_aligned.find(*iter);; ++size_iter )
                {
                    if ( *size_iter == *iter )
                    {
                        size_aligned.erase( size_iter );
                        break;
                    }
                }

                iter = start_aligned.erase( iter );
            }

            size_aligned.insert( new_empty_block );
            start_aligned.insert( new_empty_block );

            debug_print();
        }

        // Empty Memory Manager constructor ==================================
        Empty( byte* const mem, const size_t size )
            : size_aligned{ {mem, size} }, start_aligned{ {mem, size} } {}
        // ===================================================================
    };
    // Empty Memory Manager end ----------------------------------------------


    // data for pool =================================================
    byte* raw_mem;
    size_t raw_size;
    Empty free_mem;
    // ===============================================================

public:
    void debug_print() const
    {
        free_mem.debug_print();
    }

    // construct Ty on gotten mem, by constructor args.
    template< typename Ty, typename ... Args >
    Ty* alloc( Args&& ... args )
    {
        // 0 size process ( don't have to get memory. )
        if ( sizeof( Ty ) == 0 ) return new( raw_mem ) Ty( std::forward< Args >( args )... );
        // general process
        return new ( free_mem.get_mem( sizeof( Ty ) ) ) Ty( std::forward< Args >( args )... );
    }

    // return allocated memory.
    template< typename Ty >
    void dealloc( Ty*& exhausted ) noexcept
    {
        // validity check ================================================
        // nullptr is not valid for deallocation.
        if ( !exhausted ) return;
        auto exhausted_mem = reinterpret_cast< byte* >( exhausted );
        // firstly check exhausted memory is in range of this pool.
        if ( exhausted_mem < raw_mem || exhausted_mem >= raw_mem + raw_size ) return;
        // secondly check received memory is insertable.
        if ( free_mem.cannot_insertable( exhausted_mem ) ) return;
        // call destructor and invalidate pointer.
        // ===============================================================
        exhausted->~Ty();
        exhausted = nullptr;
        // 0 size process ( don't have to return memory. )
        if ( sizeof( Ty ) == 0 ) return;
        // general process
        free_mem.return_mem( exhausted_mem, sizeof( Ty ) );
    }
    
    

    // ======================================================
    // pool special member functions
    // ======================================================

    pool( const size_t mem_size )
        : raw_mem{ new byte[ mem_size ] }, raw_size{ mem_size }, free_mem{ raw_mem, mem_size }
    {
        
    }

    pool( const pool& ) = delete;
    pool& operator=( const pool& ) = delete;

    pool( pool&& other ) noexcept
        : raw_mem{ other.raw_mem }, raw_size{ other.raw_size }, free_mem{ std::move( other.free_mem ) }
    {
        other.raw_mem = nullptr;
    }

    pool& operator=( pool&& other ) noexcept
    {
        if ( this != &other )
        {
            raw_mem = other.raw_mem;
            raw_size = other.raw_size;
            free_mem = std::move( other.free_mem );
        }

        return *this;
    }

    ~pool() {}

    // ======================================================
};

#endif // _pool