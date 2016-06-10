
#include "feature.h"
#include <vector>

using namespace std;

feature feature_create( feature_type type, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h )
{
    return feature { type, w, h, xc, yc };
}

static double _rect_value( const image<double>& ii,
                           uint16_t ix, uint16_t iy,
                           uint16_t rx, uint16_t ry,
                           uint16_t rw, uint16_t rh )
{
    double value = (*ii.bits)[((iy+ry+rh-1)*ii.w)+(ix+rx+rw-1)];
    if ((ix+rx) > 0) value -= (*ii.bits)[((iy+ry+rh-1)*ii.w)+(ix+rx-1)];
    if ((iy+ry) > 0) value -= (*ii.bits)[((iy+ry-1)*ii.w)+(ix+rx+rw-1)];
    if ((ix+rx) > 0 && (iy+ry) > 0) value += (*ii.bits)[((iy+ry-1)*ii.w)+(ix+rx-1)];

    return value;
}

double feature_value( const feature& f, const image<double>& ii, uint16_t x, uint16_t y )
{
  switch(f.type) {
    case A:
        return _rect_value( ii, x, y, f.xc+(f.width/2), f.yc, f.width/2, f.height) -
               _rect_value( ii, x, y, f.xc, f.yc, f.width/2, f.height);
      break;
    case B:
        return _rect_value(ii, x, y, f.xc, f.yc, f.width, f.height/2) -
               _rect_value(ii, x, y, f.xc, f.yc+(f.height/2), f.width, f.height/2);
      break;
    case C:
        return _rect_value(ii, x, y, f.xc+(f.width/3), f.yc, f.width/3, f.height) -
               _rect_value(ii, x, y, f.xc, f.yc, f.width/3, f.height) -
               _rect_value(ii, x, y, f.xc+(f.width*2/3), f.yc, f.width/3, f.height);
      break;
    case CT:
        return _rect_value(ii, x, y, f.xc, f.yc+(f.height/3), f.width, f.height/3) -
               _rect_value(ii, x, y, f.xc, f.yc, f.width, f.height/3) -
               _rect_value(ii, x, y, f.xc, f.yc+(f.height*2/3), f.width, f.height/3);
      break;
    case D:
        return _rect_value(ii, x, y, f.xc+(f.width/2), f.yc, f.width/2, f.height/2) +
               _rect_value(ii, x, y, f.xc, f.yc+(f.height/2), f.width/2, f.height/2) -
               _rect_value(ii, x, y, f.xc+(f.width/2), f.yc+(f.height/2), f.width/2, f.height/2) -
               _rect_value(ii, x, y, f.xc, f.yc, f.width/2, f.height/2);
      break;
    default:
      break;
  }
  return 0.0;
}

void feature_scale( feature& f, double s )
{
  f.width *= s;
  f.height *= s;
  f.xc *= s;
  f.yc *= s;
}

vector<feature> generate_feature_set( uint16_t baseResolution )
{
    vector<feature> featureSet;
    uint16_t minWidth, minHeight, width, height, x, y;

    for( int type = A; type < NUM_FEATURE_TYPES; ++type )
    {
        minWidth = (type == 2) ? 3 : 4;
        width = (type == 2) ? 3 : 4;
        height = (type != 3) ? 4 : 3;

        x = 0;
        y = 0;
        while( height <= baseResolution )
        {
            while( width <= baseResolution )
            {
                while( y+height <= baseResolution )
                {
                    while( x+width <= baseResolution )
                    {
                        featureSet.push_back( feature { (feature_type)type, width, height, x, y } );
                        ++x;
                    }
                    x = 0;
                    ++y;
                }
                y = 0;
                if( type == 0 )
                    width += 2;
                else if( type == 2 )
                    width += 3;
                else ++width;
            }
            width = minWidth;
            if( type == 1 )
                height += 2;
            else if( type == 3 )
                height += 3;
            else ++height;
        }
    }

    return featureSet;
}
