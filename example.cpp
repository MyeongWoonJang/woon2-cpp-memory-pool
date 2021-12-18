#include <iostream>
#include "pool.hpp"
#include <random>

constexpr int size = 40;

std::uniform_int_distribution<> uid{ 0, size + 4 };
std::random_device rd;
std::default_random_engine dre{ rd() };

int main()
{
    pool a{ ( sizeof( int ) + sizeof( double ) ) * size };
    int* b[ size + 5 ];
    double* c[ size + 5 ];
    bool deallocated[ size + 5 ] = { false };

    for ( int i = 0; i < 4; ++i)
    {
        for ( auto& check : deallocated )
        {
            check = false;
        }

        std::cout << "Start ------------------------------------------------------\n";
        a.debug_print();

        for ( int i = 0; i < size + 5; ++i )
        {
            try
            {
                b[ i ] = a.alloc< int >( i );
                c[ i ] = a.alloc< double >( i );
                std::cout << "value: " << *b[ i ] << '\n';
                std::cout << "value: " << *c[ i ] << '\n';
            }
            catch ( std::bad_alloc )
            {
                std::cout << i << " - bad_alloc\n";
            }
        }

        for (;;)
        {
            auto val = uid( dre );

            deallocated[ val ] = true;
            a.dealloc( b[ val ] );
            a.dealloc( c[ val ] );

            bool over = true;
            for ( const auto check : deallocated )
            {
                if ( !check ) over = false;
            }

            if ( over ) break;
        }
    }
}