
#ifndef __feature_h
#define __feature_h

#include "ppm.h"

enum feature_type {
    A, B, C, CT, D, NUM_FEATURE_TYPES
};

struct feature {
    feature_type type;
    uint16_t width;
    uint16_t height;
    uint16_t xc;
    uint16_t yc;
};

feature feature_create(feature_type type, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h);
double feature_value(const feature& f, const image<double>& ii, uint16_t x, uint16_t y);
void feature_scale(feature& f, double s);

std::vector<feature> generate_feature_set(uint16_t baseResolution);

#endif
