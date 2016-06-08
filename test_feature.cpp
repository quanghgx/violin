#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "feature.h"

#include "test_ppm_data.cpp"

using namespace std;

void test_setup()
{
    FILE* outFile = fopen("car.ppm","w+b");
    for( auto v : ppm )
        fwrite( &v, 1, 1, outFile );
    fclose(outFile);
}

void test_destroy()
{
    unlink("car.ppm");
}

int main( int argc, char* argv[] )
{
    test_setup();

    {
        auto f = feature_create( A, 0, 0, 8, 8 );
        assert( f.type == A );
        assert( f.width == 8 );
        assert( f.height == 8 );
        assert( f.xc == 0 );
        assert( f.yc == 0 );
    }

    {
        auto f = feature_create( A, 0, 0, 64, 64 );

        auto img = image_create_from_ppm( "car.ppm" );
        auto lum = image_argb_to_lum<double>( img );
        auto ii = image_integral( lum );
        auto v1 = feature_value( f, ii, 0, 0 );
        feature_scale( f, 2.0 );
        auto v2 = feature_value( f, ii, 0, 0 );
        assert( v2 > v1 );
    }

    test_destroy();
}
