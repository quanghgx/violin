
#ifndef __zip_h
#define __zip_h

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


#endif
