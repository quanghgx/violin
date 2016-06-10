
#include "strong_classifier.h"

using namespace std;

strong_classifier::strong_classifier() :
    _wcs(),
    _weights(),
    _threshold(0.0)
{
}

strong_classifier::strong_classifier( const vector<weak_classifier> wcs,
                                      const vector<double> weights,
                                      double threshold ) :
    _wcs( wcs ),
    _weights( _wcs.size() ),
    _threshold( threshold )
{
    for( size_t i = 0; i < _wcs.size(); ++i )
        _weights[i] = weights[i];
}

strong_classifier::~strong_classifier() noexcept
{
}

bool strong_classifier::classify( const image<double>& img, uint16_t x, uint16_t y, double mean, double stdev)
{
    double score = 0.0;
    size_t i = 0;
    for( auto& wc : _wcs )
        score += _weights[i++] * wc.classify( img, x, y, mean, stdev );

    if( score >= _threshold )
        return true;

    return false;
}

void strong_classifier::add( const weak_classifier& wc, double weight )
{
    printf("Adding WC with weight %f\n",weight);
    _wcs.push_back( wc );
    _weights.push_back( weight );
}

void strong_classifier::scale(double s)
{
    for( auto& wc : _wcs )
        wc.scale( s );
}

void strong_classifier::optimize_threshold( const vector<image<double>>& positiveSet,
                                            double maxfnr )
{
    size_t wf;
    double thr;
    size_t positiveSetSize = positiveSet.size();
    vector<double> scores(positiveSetSize);

    for( size_t i=0; i<positiveSetSize; ++i )
    {
        scores[i] = 0.0;
        wf = 0;
        for( auto& wc : _wcs )
        {
            scores[i] += _weights[wf++] * wc.classify( positiveSet[i], 0, 0, 0.0, 1.0 );
        }
    }

    sort( scores.begin(), scores.end() );

    size_t maxfnrind = maxfnr * positiveSetSize;

    if( maxfnrind < positiveSetSize )
    {
        thr = scores[maxfnrind];
        while( maxfnrind > 0 && scores[maxfnrind] == thr )
            maxfnrind--;
        _threshold = scores[maxfnrind];
    }
}

double strong_classifier::fnr( const vector<image<double>>& positiveSet )
{
    size_t fn = 0;
    for( auto& img : positiveSet )
    {
        if( classify( img, 0, 0, 0.0, 1.0 ) == false )
            ++fn;
    }

    return ((double)fn) / ((double)positiveSet.size());
}

double strong_classifier::fpr( const vector<image<double>>& negativeSet )
{
    size_t fp = 0;
    for( auto& img : negativeSet )
    {
        if( classify( img, 0, 0, 0.0, 1.0 ) == true )
            ++fp;
    }

    return ((double)fp) / ((double)negativeSet.size());
}
