#include "weak_classifier.h"
#include "zip.h"
#include <algorithm>

using namespace std;

weak_classifier::weak_classifier( const feature& f, double threshold, bool polarity ) :
    _f( f ),
    _threshold( threshold ),
    _polarity( polarity )
{
}

weak_classifier::~weak_classifier() noexcept
{
}

double weak_classifier::find_optimum_threshold( const std::vector<double>& fvals,
                                                size_t fsize,
                                                size_t nfsize,
                                                const std::vector<double>& weights )
{
    struct score
    {
        double value;
        bool label;
        double weight;
    };

    vector<score> scores( fsize + nfsize );

    zip_transform( []( const score& s, double fvalue, double weight ) {
                       return score { fvalue, true, weight };
                   },
                   scores.begin(), scores.begin() + fsize,
                   fvals.begin(),
                   weights.begin() );

    zip_transform( []( const score& s, double fvalue, double weight ) {
                       return score{ fvalue, false, weight };
                   },
                   scores.begin() + fsize, scores.end(),
                   fvals.begin() + fsize,
                   weights.begin() + fsize );

    sort( scores.begin(), scores.end(), []( const score& s1, const score& s2 ) { return s1.value < s2.value; } );

    struct weight_sum
    {
        double sum_positive; // sum positive weights below current example S+
        double sum_negative; // sum negative weights below current example S−
    };

    vector<weight_sum> ws( fsize + nfsize );

    double totalPositive = 0.0;
    double totalNegative = 0.0;

    if( scores[0].label == false )
    {
        totalNegative = scores[0].weight;
        ws[0].sum_negative = scores[0].weight;
        ws[0].sum_positive = 0.0;
    }
    else
    {
        totalPositive = scores[0].weight;
        ws[0].sum_positive = scores[0].weight;
        ws[0].sum_negative = 0.0;
    }

    for( size_t k=1; k<(fsize+nfsize); ++k )
    {
        if (scores[k].label == false)
        {
            totalNegative += scores[k].weight;
            ws[k].sum_negative = ws[k-1].sum_negative + scores[k].weight;
            ws[k].sum_positive = ws[k-1].sum_positive;
        }
        else
        {
            totalPositive += scores[k].weight;
            ws[k].sum_positive = ws[k-1].sum_positive + scores[k].weight;
            ws[k].sum_negative = ws[k-1].sum_negative;
        }
    }

//    for( size_t k = 0; k < (fsize + nfsize); ++k )
//        printf("{%f, %f}, ",ws[k].sum_positive,ws[k].sum_negative);

    double minerror = 1.0;

    zip( [&]( const weight_sum& weightSum, score s ) {
        double errorp = weightSum.sum_positive + totalNegative - weightSum.sum_negative;
        double errorm = weightSum.sum_negative + totalPositive - weightSum.sum_positive;
        if (errorp < errorm)
        {
            if (errorp < minerror)
            {
                minerror = errorp;
                _threshold = s.value;
                _polarity = false;
            }
        }
        else
        {
            if (errorm < minerror)
            {
                minerror = errorm;
                _threshold = s.value;
                _polarity = true;
            }
        }
     },
     ws.begin(), ws.end(),
     scores.begin() );

    return minerror;
}

int weak_classifier::classify( const image<double>& img,
                               uint16_t x,
                               uint16_t y,
                               double mean,
                               double stdev )
{
    double fval = feature_value( _f, img, x, y );

    if( _f.type == C || _f.type == CT )
        fval += _f.width * _f.height * mean / 3;

    if( stdev != 0.0 )
        fval /= stdev;

    if (fval < _threshold)
        return (_polarity) ? 1 : -1;
    else return (_polarity) ? -1 : 1;
}

void weak_classifier::scale( double s )
{
    feature_scale( _f, s );
    _threshold *= pow(s, 2);
}
