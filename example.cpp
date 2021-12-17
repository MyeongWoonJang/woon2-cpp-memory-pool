#include <iostream>
#include "pool.hpp"

int main()
{
    pool a{ sizeof(int) * 100 };
    int* b[ 105 ];

    for ( int i = 0; i < 105; ++i )
    {
        try
        {
            b[i] = a.alloc< int >( i );
            std::cout << "value: " << *b[i] << '\n';
        }
        catch ( std::bad_alloc )
        {
            std::cout << i + 1 << " - bad_alloc\n";
        }
    }

    for ( int i = 0; i < 105; ++i )
    {
        a.dealloc( b[i] );
    }

    for ( int i = 0; i < 105; ++i )
    {
        try
        {
            b[ i ] = a.alloc< int >( i );
            std::cout << "value: " << *b[ i ] << '\n';
        }
        catch ( std::bad_alloc )
        {
            std::cout << i + 1 << " - bad_alloc\n";
        }
    }

    for ( int i = 0; i < 105; ++i )
    {
        a.dealloc( b[ i ] );
    }
}