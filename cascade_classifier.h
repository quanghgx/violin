
#ifndef __cascade_classifier_h
#define __cascade_classifier_h

#include "strong_classifier.h"

using namespace std;

class cascade_classifier
{
public:
    cascade_classifier( uint16_t baseResolution );
    ~cascade_classifier() noexcept;

    bool classify( const image<double>& img, uint16_t x, uint16_t y, double mean, double stdev );
    void push_back( const strong_classifier& sc ) { _sc.push_back( sc ); }
    void pop_back() { _sc.pop_back(); };
    double fnr( const std::vector<image<double>>& positiveSet );
    double fpr( const std::vector<image<double>>& negativeSet );
    void strictness( double p );
    uint16_t get_base_resolution() { return _baseResolution; }
    void scale( double s );
    std::vector<strong_classifier> get_strong_classifiers() { return _sc; }

private:
    std::vector<strong_classifier> _sc;
    uint16_t _baseResolution;
};

#endif
