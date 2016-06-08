
#ifndef __strong_classifier_h
#define __strong_classifier_h

#include <vector>
#include "weak_classifier.h"
#include "ppm.h"

class strong_classifier
{
public:
    strong_classifier();
    strong_classifier( const std::vector<weak_classifier> wcs,
                       const std::vector<double> weights,
                       double threshold );
    ~strong_classifier() noexcept;

    bool classify( const image<double>& img, uint16_t x, uint16_t y, double mean, double stdev );
    void add( const weak_classifier& wc, double weight );
    void scale( double s );
    void optimize_threshold( const std::vector<image<double>>& positiveSet, double maxfnr );
    double fnr( const std::vector<image<double>>& positiveSet );
    double fpr( const std::vector<image<double>>& negativeSet );
    void strictness( double p ) { _threshold *= p; }
    std::vector<weak_classifier> get_weak_classifiers() { return _wcs; }

protected:
    std::vector<weak_classifier> _wcs;
    std::vector<double> _weights;
    double _threshold;
};

#endif
