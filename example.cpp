#include <iostream>
#include "pool.hpp"
#include "pool.h"
#include <random>
#include <chrono>

constexpr int size = 40;

std::uniform_int_distribution<> uid{ 0, size + 4 };
std::random_device rd;
std::default_random_engine dre{ rd() };

template < typename time_t = std::chrono::milliseconds, typename precision_t = std::chrono::nanoseconds, typename Func, typename ... Args >
auto count_time( Func func, Args&& ... args )
{
    using std::chrono::system_clock;
    using std::chrono::duration_cast;

    auto start = system_clock::now();
    func( std::forward< Args >( args )... );

    return duration_cast< precision_t >( system_clock::now() - start ).count()
        / static_cast< long double >( precision_t::period::den ) * time_t::period::den;
}

constexpr size_t obj_size = 40;
constexpr size_t obj_cnt = 100000;

template < size_t Size >
struct Data
{
    char impl[ Size ];
};

using Data_t = Data< obj_size >;

pool flexible_pool{ sizeof( Data_t ) * obj_cnt };
Pool< Data_t > fixed_pool{ obj_cnt };

Data_t* raw_pointers[ obj_cnt ];

void test_raw()
{
    for ( int i = 0; i < obj_cnt; ++i )
    {
        raw_pointers[ i ] = new Data_t;
    }

    for ( int i = 0; i < obj_cnt; ++i )
    {
        delete raw_pointers[ i ];
    }
}

void test_flexible_pool()
{
    for ( int i = 0; i < obj_cnt; ++i )
    {
        raw_pointers[ i ] = flexible_pool.alloc< Data_t >();
    }

    for ( int i = 0; i < obj_cnt; ++i )
    {
        flexible_pool.dealloc( raw_pointers[ i ] );
    }
}

void test_fixed_pool()
{
    for ( int i = 0; i < obj_cnt; ++i )
    {
        raw_pointers[ i ] = fixed_pool.ralloc();
    }

    for ( int i = 0; i < obj_cnt; ++i )
    {
        fixed_pool.dealloc( raw_pointers[ i ] );
    }
}

int main()
{
    std::cout << "[ test_raw ] : " <<
        count_time( test_raw ) << " ms taken.\n";

    std::cout << "[ test_flexible_pool ] : " <<
        count_time( test_flexible_pool ) << " ms taken.\n";

    std::cout << "[ test_fixed_pool ] : " <<
        count_time( test_fixed_pool ) << " ms taken.\n";
}