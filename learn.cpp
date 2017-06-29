#include "ppm.h"
#include "feature.h"
#include "weak_classifier.h"
#include "strong_classifier.h"
#include "cascade_classifier.h"
#include "utils.h"
#include <assert.h>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <cmath>

using namespace std;

struct image_resources {
    image<uint32_t> img;
    uint16_t sw;
    uint16_t sh;
    image<uint32_t> scaled;
    image<uint32_t> cropped;
    image<double> lum;
    image<double> normalized;
    image<double> integral;
    image<double> mirror;
};

void load_dataset(const string& path,
        uint16_t baseResolution,
        vector<image_resources>& positive,
        vector<image_resources>& negative) {
    printf("Loading dataset from %s...\n", path.c_str());

    vector<pair<string, vector<image_resources>&>> sources = {
        {"/positive", positive},
        {"/negative", negative}
    };

    for (auto& s : sources) {
        auto ppmPaths = get_ppm_file_paths(path + s.first);

        for (auto& p : ppmPaths) {
            image_resources ir;
            ir.img = image_create_from_ppm(p);

            double imgAR = (double) ir.img.w / (double) ir.img.h;

            // image is wider than tall
            if (imgAR > 1.0) {
                ir.sw = baseResolution;
                ir.sh = (uint16_t) (baseResolution / imgAR);
            } else // image is taller than wide
            {
                ir.sw = (uint16_t) (baseResolution * imgAR);
                ir.sh = baseResolution;
            }

            ir.scaled = image_resize(ir.img, ir.sw, ir.sh);

            ir.cropped = image_create<uint32_t>(baseResolution, baseResolution);

            if (imgAR > 1.0) {
                uint16_t heightDelta = baseResolution - ir.sh;
                image_blit<uint32_t>(ir.scaled, 0, 0, ir.sw, ir.sh, ir.cropped, 0, heightDelta / 2);
            } else {
                uint16_t widthDelta = baseResolution - ir.sw;
                image_blit<uint32_t>(ir.scaled, 0, 0, ir.sw, ir.sh, ir.cropped, widthDelta / 2, 0);
            }

            ir.lum = image_argb_to_lum<double>(ir.cropped);
            ir.normalized = image_normalize(ir.lum);
            ir.integral = image_integral(ir.normalized);
            ir.mirror = image_mirror_vertical(ir.integral);
            s.second.push_back(ir);
        }
    }
}

vector<image<double>> slice_dataset_integral(const vector<image_resources>& resources) {
    vector<image<double>> images;
    for (auto& r : resources) {
        images.push_back(r.integral);
        images.push_back(r.mirror);
    }
    return images;
}

strong_classifier adaboost_learning(cascade_classifier& cc,
        const vector<feature>& features,
        const vector<image<double>>&trainPositive,
        const vector<image<double>>&trainNegative,
        const vector<image<double>>&validation,
        double minfpr,
        double maxfnr,
        uint16_t baseResolution) {
    size_t trainPositiveSize = trainPositive.size();
    size_t trainNegativeSize = trainNegative.size();

    vector<double> weights(trainPositiveSize + trainNegativeSize);

    for (size_t i = 0; i < trainPositiveSize; ++i)
        weights[i] = 1.0 / double(2 * trainPositiveSize);

    for (size_t i = 0; i < trainNegativeSize; ++i)
        weights[trainPositiveSize + i] = 1.0 / double(2 * trainNegativeSize);

    strong_classifier sc;

    double cfpr = 1.0;
    vector<double> fvalues(trainPositiveSize + trainNegativeSize);
    while (cfpr > minfpr) {
        if (sc.fpr(trainNegative) == 0)
            break; // all training negative samples classified correctly. could not achieve validation target

        double wsum = accumulate(weights.begin(), weights.end(), 0.0);
        transform(weights.begin(), weights.end(), weights.begin(), [wsum](double v) {
            return v / wsum; });

        double minerror = 1.0;
        weak_classifier bestwc;
        for (auto& f : features) {
            weak_classifier wc(f);

#if 0
            auto b = fvalues.begin();
            parallel_for(b, b + trainPositiveSize,
                    [f, &trainPositive, b](vector<double>::iterator i) {
                        *i = feature_value(f, trainPositive[i - b], 0, 0);
                    });

            b = fvalues.begin() + trainPositiveSize;
            parallel_for(b, b + trainNegativeSize,
                    [f, &trainNegative, b](vector<double>::iterator i) {
                        *i = feature_value(f, trainNegative[i - b], 0, 0);
                    });
#endif
            for (size_t i = 0; i < trainPositiveSize; ++i)
                fvalues[i] = feature_value(f, trainPositive[i], 0, 0);
            for (size_t i = 0; i < trainNegativeSize; ++i)
                fvalues[trainPositiveSize + i] = feature_value(f, trainNegative[i], 0, 0);
            double wcerror = wc.find_optimum_threshold(fvalues, trainPositiveSize, trainNegativeSize, weights);
            if (wcerror < minerror) {
                printf("wcerror = %.32f\n", wcerror);
                bestwc = wc;
                minerror = wcerror;
            }
        }

        double betat = minerror / (1.0 - minerror);
        printf("betat = %f\n", betat);
        for (size_t i = 0; i < trainPositiveSize; ++i)
            if (bestwc.classify(trainPositive[i], 0, 0, 0, 1) == 1)
                weights[i] *= betat;

        for (size_t i = 0; i < trainNegativeSize; ++i)
            if (bestwc.classify(trainNegative[i], 0, 0, 0, 1) == -1)
                weights[trainPositiveSize + i] *= betat;

        sc.add(bestwc, log(1.0 / betat));
        sc.optimize_threshold(trainPositive, maxfnr);

        cc.push_back(sc);
        cfpr = cc.fpr(validation);
        cc.pop_back();

        printf("Added new Haar feature (FPR=%f)\n", cfpr);
    }

    return sc;
}

const uint16_t BASE_RES_W = 32;
const uint16_t BASE_RES_H = 32;

int main(int argc, char* argv[]) {
    //
    // To DO
    //
    // - Currently doing aspect incorrect resize on images
    //   - Compute mean aspect ratio (MAR) for dataset
    //   - Compute feature resolution using MAR
    //   - optionally crop or pad images that need it
    //
    // - Need some storage mechanism for cascade classifier
    //   - features can be regenerated, but we need to store
    //     the features that were added to the cascade and
    //     their associated weights
    //

    vector<image_resources> trainPositive, trainNegative;
    vector<image_resources> testPositive, testNegative;

    load_dataset("train", BASE_RES_W, trainPositive, trainNegative);
    printf("  %lu positive samples loaded...\n", trainPositive.size());
    printf("  %lu negative samples loaded...\n", trainNegative.size());

    load_dataset("test", BASE_RES_W, testPositive, testNegative);
    printf("  %lu positive samples loaded...\n", testPositive.size());
    printf("  %lu negative samples loaded...\n", testNegative.size());

    auto features = generate_feature_set(BASE_RES_W);
    printf("Resolution %uX%u creates %lu features.\n", BASE_RES_W, BASE_RES_H, features.size());

    printf("Boosting...\n");

    cascade_classifier cc(BASE_RES_W);

    auto trainPositiveIntegrals = slice_dataset_integral(trainPositive);
    auto trainNegativeIntegrals = slice_dataset_integral(trainNegative);
    auto trainVerifyIntegrals = slice_dataset_integral(trainNegative);

    double fprGoals[]{0.70, 0.60, 0.50};

    strong_classifier sc;

    for (auto goal : fprGoals) {
        printf("goal: %f\n", goal);
        fflush(stdout);

        double fpr = cc.fpr(trainNegativeIntegrals);
        printf("cc fpr: %f\n", fpr);
        if (fpr < goal) {
            printf("    fpr < goal, skipping round.\n");
            continue;
        }

        vector<image<double>> misclassifiedNegativeIntegrals;
        for (auto ni : trainNegativeIntegrals)
            if (cc.classify(ni, 0, 0, 0.0, 1.0) == true)
                misclassifiedNegativeIntegrals.push_back(ni);

        if (misclassifiedNegativeIntegrals.empty() || trainPositiveIntegrals.empty()) {
            printf("No training data left!\n");
            continue;
        }

        sc = adaboost_learning(cc,
                features,
                trainPositiveIntegrals,
                misclassifiedNegativeIntegrals,
                trainVerifyIntegrals,
                goal,
                0.01,
                BASE_RES_W);

        cc.push_back(sc);

        size_t fdel = 0, nfdel = 0;
        for (size_t n = 0; n < trainNegativeIntegrals.size(); ++n) {
            if (sc.classify(trainNegativeIntegrals[n], 0, 0, 0.0, 1.0) == false) {
                trainNegativeIntegrals.erase(trainNegativeIntegrals.begin() + n);
                n--;
                nfdel++;
            }
        }

        for (size_t n = 0; n < trainPositiveIntegrals.size(); n++) {
            if (sc.classify(trainPositiveIntegrals[n], 0, 0, 0.0, 1.0) == false) {
                trainPositiveIntegrals.erase(trainPositiveIntegrals.begin() + n);
                n--;
                fdel++;
            }
        }

        printf("Removed %lu negatives we got right.\n", nfdel);
    }

    auto testPositiveIntegrals = slice_dataset_integral(testPositive);
    auto testNegativeIntegrals = slice_dataset_integral(testNegative);

    size_t numCorrect = 0;
    for (auto& vi : testPositiveIntegrals)
        if (cc.classify(vi, 0, 0, 0.0, 1.0) == true)
            ++numCorrect;

    printf("numCorrect (positive) = %lu\n", numCorrect);
    fflush(stdout);

    for (auto& vi : testNegativeIntegrals)
        if (cc.classify(vi, 0, 0, 0.0, 1.0) == false)
            ++numCorrect;

    printf("total numCorrect = %lu\n", numCorrect);

    printf("Accuracy: %f\n", (double) numCorrect / (double) (testPositiveIntegrals.size() + testNegativeIntegrals.size()));

    return 0;
}

