
#include "zip.h"
#include <vector>

// template<typename Iterator, class Function>
// void parallel_for( const Iterator& first, const Iterator& last, Function&& f, const size_t nthreads = std::thread::hardware_concurrency(), const size_t threshold = 1 )

using namespace std;

int main( int argc, char* argv[] )
{
    vector<int> vs = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    parallel_for( vs.begin(), vs.end(), []( vector<int>::iterator v ) {
        printf("%d ",*v);
    });
}
