#ifndef _spool
#define _spool

#include <memory>
#include <utility>
#include <array>
#include <type_traits>
#include <algorithm>

template < typename Ty >
class Pool
{
public:
	class dealloc_func
	{
	public:
		dealloc_func() = default;
		dealloc_func( Pool* pl ) : pl{ pl } {}
		dealloc_func( const dealloc_func& ) = default;
		dealloc_func& operator=( const dealloc_func& ) = default;
		dealloc_func( dealloc_func&& ) noexcept = default;
		dealloc_func& operator=( dealloc_func&& ) noexcept = default;

		void operator()( Ty* exhausted ) noexcept
		{
			pl->dealloc( exhausted );
		}

	private:
		Pool* pl;
	};

	using byte = char;
	using byte_ptr = char*;
	using byte_pptr = char**;
	using Uptr = std::unique_ptr< Ty, dealloc_func >;
	using Sptr = std::shared_ptr< Ty >;

	// requires constructor args
	// returns an unique pointer
	// automatically deallocate
	template < typename ... Args >
	Uptr alloc( Args&& ... args )
	{
		return alloc_impl< Uptr >( std::forward< Args >( args )... );
	}

	// requires constructor args
	// returns a shared pointer
	// automatically deallocate
	template < typename ... Args >
	Sptr salloc( Args&& ... args )
	{
		return alloc_impl< Sptr >( std::forward< Args >( args )... );
	}

	// requires constructor args
	// returns a raw pointer
	template < typename ... Args >
	Ty* ralloc( Args&& ... args )
	{
		return alloc_impl< Rptr >( std::forward< Args >( args )... );
	}

	// requires array size template args, constructor args
	// returns array of unique pointer
	// automatically deallocate
	template < size_t arr_size, typename ... Args >
	std::array< Uptr, arr_size > alloc( Args&& ... args )
	{
		return alloc_array_impl< Uptr >( std::make_index_sequence< arr_size >{}, std::forward< Args >( args )... );
	}

	// requires array size template args, constructor args
	// returns array of shared pointer
	// automatically deallocate
	template < size_t arr_size, typename ... Args >
	std::array< Sptr, arr_size > salloc( Args&& ... args )
	{
		return alloc_array_impl< Sptr >( std::make_index_sequence< arr_size >{}, std::forward< Args >( args )... );
	}

	// requires array size template args, constructor args
	// returns array of raw pointer
	template < size_t arr_size, typename ... Args >
	std::array< Ty*, arr_size > ralloc( Args&& ... args )
	{
		return alloc_array_impl< Rptr >( std::make_index_sequence< arr_size >{}, std::forward< Args >( args )... );
	}

	// requires object's pointer to deallocate
	// It doesn't do anything if the pointer was already deallocated.
	// only for raw pointer
	void dealloc( Ty*& exhausted ) noexcept
	{
		if ( exhausted )
		{
			exhausted->~Ty();
			putmem( reinterpret_cast< byte_ptr >( exhausted ) );
			exhausted = nullptr;
			++avl_cnt;
		}
	}

	const size_t available_cnt() const noexcept
	{
		return avl_cnt;
	}

	Pool( const size_t capacity ) : mem{ new byte[ safe_mem() * capacity + sizeof( void* ) ] },
		free_ptr{ mem }, avl_cnt{ capacity }, ref_cnt{ 1 }
	{
		// write next memory's address in each memory
		byte_pptr cur = reinterpret_cast< byte_pptr >( mem );
		byte_ptr next = mem;

		for ( size_t i = 0; i < capacity; ++i )
		{
			next += safe_mem();
			*cur = next;
			cur = reinterpret_cast< byte_pptr >( next );
		}

		*cur = nullptr;
	}

	Pool( const pool& other ) = delete;
	Pool& operator=( const pool& other ) = delete;

	~Pool()
	{
		if ( mem )
		{
			if ( !--ref_cnt )
			{
				delete[] mem;
			}
		}
	}

private:
	// for safe memory writing, each memory size must be over sizeof( void* ).
	static constexpr const size_t safe_mem() noexcept
	{
		return std::max( sizeof( Ty ), sizeof( void* ) );
	}

	template < typename Ptr_t, typename ... Args >
	auto alloc_impl( Args ... args )
	{
		check_avl_cnt();
		Ptr_t ret{ create_obj( std::forward< Args >( args )... ), dealloc_func{ this } };
		--avl_cnt;

		return ret;
	}

	template < typename Ptr_t, typename ... Args, size_t ... Idx >
	auto alloc_array_impl( std::index_sequence< Idx... >, Args&& ... args )
	{
		return std::array<
			std::conditional_t< std::is_same_v< Ptr_t, Rptr >, Ty*, Ptr_t >,
			sizeof...( Idx )
		> { _dummy( alloc_impl< Ptr_t >( std::forward< Args >( args )... ), Idx )... };
	}

	void check_avl_cnt()
	{
		if ( !avl_cnt )
		{
			throw std::bad_alloc{};
		}
	}

	template < typename ... Args >
	Ty* create_obj( Args ... args )
	{
		return new( getmem() ) Ty{ std::forward< Args >( args )... };
	}

	byte_ptr getmem() noexcept
	{
		byte_ptr ret = free_ptr;
		free_ptr = *reinterpret_cast< byte_pptr >( free_ptr );
		return ret;
	}

	void putmem( byte_ptr mem ) noexcept
	{
		*reinterpret_cast< byte_pptr >( mem ) = free_ptr;
		free_ptr = mem;
	}

	template < typename Tx >
	decltype(auto) _dummy( Tx&& elem, size_t ) const noexcept
	{
		return std::forward< Tx >( elem );
	}

	// for template, raw pointer uselessly require dealloc_func
	struct Rptr
	{
		Rptr( Ty* inst, dealloc_func ) : inst{ inst } {}
		operator Ty*() { return inst; }

		Ty* inst;
	};

	byte_ptr mem;
	byte_ptr free_ptr;
	size_t avl_cnt;
	size_t ref_cnt;
};

template < typename Ty >
using pool_ptr = typename Pool< Ty >::Uptr;

template < typename Ty >
using pool_sptr = typename Pool< Ty >::Sptr;

#endif