
#ifndef __zip_h
#define __zip_h

#include <future>
#include <cstdlib>
#include <vector>
#include <algorithm>

template<typename iterator>
void advance_all( iterator& i )  { ++i; }

template<typename iterator, typename ... iterators>
void advance_all( iterator & i, iterators& ... ni )
{
    ++i;
    advance_all( ni... );
}

template<typename function, typename iterator, typename ... iterators>
function zip( function func, iterator begin, iterator end, iterators ... ni )
{
    for(;begin != end; ++begin, advance_all(ni...))
        func(*begin, *(ni)... );
    return func;
}

template<typename function, typename iterator, typename ... iterators>
function zip_transform( function func, iterator begin, iterator end, iterators ... ni )
{
    for(;begin != end; ++begin, advance_all(ni...))
        *begin = func(*begin, *(ni)... );
    return func;
}

template<typename iterator, class function>
void parallel_for( const iterator& first, const iterator& last, function&& f, const size_t nthreads = std::thread::hardware_concurrency(), const size_t threshold = 1 )
{
    const size_t portion = std::max( threshold, (last-first) / nthreads );
    std::vector<std::thread> threads;
    for ( iterator it = first; it < last; it += portion )
    {
        iterator begin = it;
        iterator end = it + portion;
        if ( end > last )
            end = last;

        threads.push_back( std::thread( [=,&f]() {
            for ( iterator i = begin; i != end; ++i )
                f(i);
        }));
    }
    std::for_each(threads.begin(), threads.end(), [](std::thread& x){x.join();});
}

#endif
