#include "ppm.h"
#include "feature.h"
#include "weak_classifier.h"
#include "strong_classifier.h"
#include "cascade_classifier.h"
#include "utils.h"
#include <assert.h>
using namespace std;

struct image_resources
{
    image<uint32_t> img;
    uint16_t sw;
    uint16_t sh;
    image<uint32_t> scaled;
    image<uint32_t> cropped;
    image<double> lum;
    image<double> integral;
};

void load_dataset( const string& path,
                   uint16_t baseResolution,
                   vector<image_resources>& positive,
                   vector<image_resources>& negative )
{
    printf("Loading dataset from %s...\n",path.c_str());

    vector<pair<string,vector<image_resources>&>> sources = { {"/positive", positive}, {"/negative", negative} };

    for( auto& s : sources )
    {
        auto ppmPaths = get_ppm_file_paths( path + s.first );

        for( auto& p : ppmPaths )
        {
            image_resources ir;
            ir.img = image_create_from_ppm( p );

            double imgAR = (double)ir.img.w / (double)ir.img.h;

            // image is wider than tall
            if( imgAR > 1.0 )
            {
                ir.sw = baseResolution;
                ir.sh = (uint16_t)(baseResolution / imgAR);
            }
            else // image is taller than wide
            {
                ir.sw = (uint16_t)(baseResolution * imgAR);
                ir.sh = baseResolution;
            }

            ir.scaled = image_resize( ir.img, ir.sw, ir.sh );

            ir.cropped = image_create<uint32_t>( baseResolution, baseResolution );

            if( imgAR > 1.0 )
            {
                uint16_t heightDelta = baseResolution - ir.sh;
                image_blit<uint32_t>( ir.scaled, 0, 0, ir.sw, ir.sh, ir.cropped, 0, heightDelta / 2 );
            }
            else
            {
                uint16_t widthDelta = baseResolution - ir.sw;
                image_blit<uint32_t>( ir.scaled, 0, 0, ir.sw, ir.sh, ir.cropped, widthDelta / 2, 0 );
            }

            ir.lum = image_argb_to_lum<double>( ir.cropped );
            auto norm = image_normalize( ir.lum );
            ir.integral = image_integral( norm );
            s.second.push_back( ir );
        }
    }
}

vector<image<double>> slice_dataset_integral( const vector<image_resources>& resources )
{
    vector<image<double>> images;
    for( auto& r : resources )
        images.push_back( r.integral );
    return images;
}

strong_classifier adaboost_learning( cascade_classifier& cc,
                                     const vector<feature>& features,
                                     const vector<image<double>>& trainPositive,
                                     const vector<image<double>>& trainNegative,
                                     const vector<image<double>>& validation,
                                     double minfpr,
                                     double maxfnr,
                                     uint16_t baseResolution )
{
    size_t trainPositiveSize = trainPositive.size();
    size_t trainNegativeSize = trainNegative.size();

    vector<double> weights(trainPositiveSize + trainNegativeSize);

    for( size_t i=0; i<trainPositiveSize; ++i)
        weights[i] = 1.0/double(2*trainPositiveSize);

    for( size_t i=0; i<trainNegativeSize; ++i)
        weights[trainPositiveSize+i] = 1.0/double(2*trainNegativeSize);

    strong_classifier sc;

    double cfpr = 1.0;
    vector<double> fvalues(trainPositiveSize + trainNegativeSize);
    while( cfpr > minfpr )
    {
        if( sc.fpr( trainNegative ) == 0 )
        {
            printf("All training samples correctly classified. Could not achieve target FPR.\n");
            break; // all training negative samples classified correctly. could not achieve validation target
        }
        double wsum = accumulate( weights.begin(), weights.end(), 0.0 );
        printf("wsum = %f\n",wsum);
        transform( weights.begin(), weights.end(), weights.begin(), [wsum](double v){ return v / wsum; } );
        double wsum2 = accumulate( weights.begin(), weights.end(), 0.0 );
        printf("wsum2 = %f\n",wsum2);
        
        double minerror = 1.0;
        weak_classifier bestwc;
        for( auto& f : features )
        {
            weak_classifier wc( f );

            //auto r1 = async( launch::async,
              //               [&](){
                //                 for( size_t i = 0; i < (trainPositiveSize/2); ++i )
                  //                  fvalues[i] = feature_value( f, trainPositive[i], 0, 0 );
                    //         } );

#if 1
            for( size_t i = 0; i < trainPositiveSize; ++i )
                fvalues[i] = feature_value( f, trainPositive[i], 0, 0 );
            for( size_t i = 0; i < trainNegativeSize; ++i )
                fvalues[trainPositiveSize+i] = feature_value( f, trainNegative[i], 0, 0 );
#endif
            double wcerror = wc.find_optimum_threshold( fvalues, trainPositiveSize, trainNegativeSize, weights );
            if( wcerror < minerror )
            {
                printf("wcerror = %.32f\n",wcerror);
                bestwc = wc;
                minerror = wcerror;
            }
        }

        double betat = minerror / (1.0-minerror);
        printf("betat = %f\n",betat);
        for( size_t i = 0; i < trainPositiveSize; ++i )
            if( bestwc.classify(trainPositive[i], 0, 0, 0, 1) == 1 )
                weights[i] *= betat;

        for( size_t i = 0; i < trainNegativeSize; ++i )
            if( bestwc.classify(trainNegative[i], 0, 0, 0, 1) == -1 )
                weights[trainPositiveSize+i] *= betat;

        sc.add( bestwc, log(1.0/betat) );
        sc.optimize_threshold( trainPositive, maxfnr );

        cc.push_back( sc );
        cfpr = cc.fpr( validation );
        cc.pop_back();

        printf("Added new Haar feature (FPR=%f)\n",cfpr);

    }

    return sc;
}

const uint16_t BASE_RES_W = 32;
const uint16_t BASE_RES_H = 32;

int main( int argc, char* argv[] )
{
    //
    // To Do
    //
    // - Need some storage mechanism for cascade classifier
    //   - features can be regenerated, but we need to store
    //     the features that were added to the cascade and
    //     their associated weights
    //

    vector<image_resources> trainP, trainN;
    load_dataset( "train", BASE_RES_W, trainP, trainN );
    printf("  %lu positive samples loaded...\n", trainP.size());
    printf("  %lu negative samples loaded...\n", trainN.size());

    vector<image_resources> testP, testN;
    load_dataset( "test", BASE_RES_W, testP, testN );
    printf("  %lu positive samples loaded...\n", testP.size());
    printf("  %lu negative samples loaded...\n", testN.size());

    auto features = generate_feature_set( BASE_RES_W );
    printf("Resolution %uX%u creates %lu features.\n",BASE_RES_W,BASE_RES_H,features.size());

    printf("Boosting...\n");

    cascade_classifier cc( BASE_RES_W );

    auto trainPI = slice_dataset_integral(trainP);
    auto trainNI = slice_dataset_integral(trainN);
    auto trainVI = slice_dataset_integral(testN);

    double fprGoals[] { 0.85, 0.70, 0.55, 0.40, 0.30, 0.25, 0.20 };

    for( auto goal : fprGoals )
    {
        printf("goal: %f\n",goal);
        fflush(stdout);
        
        vector<image<double>> negatives;

        copy_if( trainNI.begin(), trainNI.end(),
                 back_inserter(negatives),
                 [&]( const image<double>& img ) { return cc.classify( img, 0, 0, 0.0, 1.0 ); } );

        strong_classifier sc = adaboost_learning( cc,
                                                  features,
                                                  trainPI,
                                                  negatives,
                                                  trainVI,
                                                  goal,
                                                  0.01,
                                                  BASE_RES_W );

        cc.push_back( sc );

        size_t fdel = 0, nfdel=0;
        for( size_t n=0; n<trainNI.size(); ++n )
        {
            if(sc.classify(trainNI[n], 0, 0, 0.0, 1.0) == false)
            {
                trainNI.erase(trainNI.begin()+n);
                n--;
                nfdel++;
            }
        }
        for( size_t n=0; n<trainPI.size(); n++)
        {
            if (sc.classify(trainPI[n], 0, 0, 0.0, 1.0) == false)
            {
                trainPI.erase(trainPI.begin()+n);
                n--;
                fdel++;
            }
        }
    }

    auto testPI = slice_dataset_integral(testP);
    auto testNI = slice_dataset_integral(testN);

    size_t numCorrect = 0;
    for( auto& vi : testPI )
        if( cc.classify( vi, 0, 0, 0.0, 1.0 ) == true )
            ++numCorrect;

    printf("numCorrect (positive) = %lu\n",numCorrect);
    fflush(stdout);

    for( auto& vi : testNI )
        if( cc.classify( vi, 0, 0, 0.0, 1.0 ) == false )
            ++numCorrect;

    printf("total numCorrect = %lu\n",numCorrect);

    printf("Accuracy: %f\n",(double)numCorrect / (double)(testPI.size() + testNI.size()));

    return 0;
}

