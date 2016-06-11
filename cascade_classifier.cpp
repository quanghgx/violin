
#include "cascade_classifier.h"

using namespace std;

cascade_classifier::cascade_classifier( uint16_t baseResolution ) :
    _sc(),
    _baseResolution(baseResolution)
{
}

cascade_classifier::~cascade_classifier() noexcept
{
}

bool cascade_classifier::classify( const image<double>& img, uint16_t x, uint16_t y, double mean, double stdev )
{
    for( auto& sc : _sc )
    {
        if( sc.classify( img, x, y, mean, stdev ) == false )
            return false;
    }

    return true;
}

double cascade_classifier::fnr( const std::vector<image<double>>& positiveSet )
{
    size_t fn = 0;
    for( auto& img : positiveSet )
    {
        if( classify( img, 0, 0, 0.0, 1.0 ) == false )
            ++fn;
    }

    return ((double)fn) / ((double)positiveSet.size());
}

double cascade_classifier::fpr( const std::vector<image<double>>& negativeSet )
{
    size_t fp = 0;
    for( auto& img : negativeSet )
    {
        if( classify( img, 0, 0, 0.0, 1.0 ) == true )
            ++fp;
    }

    return ((double)fp) / ((double)negativeSet.size());
}

void cascade_classifier::strictness( double p )
{
    for( auto& sc : _sc )
        sc.strictness( p );
}

void cascade_classifier::scale( double s )
{
    _baseResolution *= s;
    for( auto& sc : _sc )
        sc.scale( s );
}
