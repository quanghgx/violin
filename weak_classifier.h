
#ifndef __weak_classifier_h
#define __weak_classifier_h

#include "feature.h"
#include "ppm.h"
#include <string>
#include <vector>

class weak_classifier
{
public:
    weak_classifier( const feature& f = feature(), double threshold=0.0, bool polarity=true );
    ~weak_classifier() noexcept;

    double find_optimum_threshold( const std::vector<double>& fvals,
                                   size_t fsize,
                                   size_t nfsize,
                                   const std::vector<double>& weights );

    int classify( const image<double>& img,
                  uint16_t x,
                  uint16_t y,
                  double mean,
                  double stdev );

    void scale( double s );

private:
    feature _f;
    double _threshold;
    bool _polarity;
};

#endif
