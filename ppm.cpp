
#include "ppm.h"
#include <stdlib.h>

using namespace std;

static string _ppm_read_header_line(FILE* inFile);

image<uint32_t> image_create_from_ppm(const std::string& fileName) {
    FILE* inFile = fopen(fileName.c_str(), "rb");
    if (!inFile)
        throw runtime_error("Unable to open ppm file.");

    image<uint32_t> img;

    try {
        std::string line = _ppm_read_header_line(inFile);

        if (line != "P6")
            throw runtime_error("Invalid signature in ppm file.");

        line = _ppm_read_header_line(inFile);

        size_t spaceIndex = line.find(" ");
        if (spaceIndex == string::npos)
            throw runtime_error("Invalid ppm file. Cannot parse width.");
        if ((line.length() - spaceIndex) <= 1)
            throw runtime_error("Invalid ppm file. Cannot parse height.");

        img.w = (uint16_t) stoul(line.substr(0, spaceIndex));
        if (img.w > 16384)
            throw runtime_error("ppm too wide.");

        img.h = (uint16_t) stoul(line.substr(spaceIndex + 1));
        if (img.h > 16384)
            throw runtime_error("ppm too tall.");

        line = _ppm_read_header_line(inFile);
        uint16_t colorMax = (uint16_t) stoul(line);

        if (colorMax > 255)
            throw runtime_error("ppm support limited to 24 bit rgb.");

        img.bits = make_shared<vector < uint32_t >> (img.w * img.h);

        uint32_t* begin = &img.bits->at(0);
        uint32_t* dst = begin;

        while (!feof(inFile) && ((dst - begin) < (img.w * img.h))) {
            uint32_t word = UINT32_MAX;
            fread(((uint8_t*) & word) + 1, 1, 3, inFile);
            *dst++ = ntohl(word);

#if 0
            *(dst + 3) = 255;

            fread(dst + 2, 1, 1, inFile);

            fread(dst + 1, 1, 1, inFile);

            fread(dst + 0, 1, 1, inFile);

            dst += 4;
#endif
        }

        fclose(inFile);
    } catch (...) {
        if (inFile)
            fclose(inFile);
        throw;
    }

    return img;
}

// reads lines from a ppm file. if a line starts with a # it is ignored.

static string _ppm_read_header_line(FILE* inFile) {
    uint8_t headerLine[1024];
    string line;
    while (!feof(inFile) && line.length() == 0) {
        string tmp = fgets((char*) headerLine, 1024, inFile);
        if (!tmp.empty()) {
            const string delims(" \t\r\n");
            auto pos = tmp.find_first_not_of(delims);
            if (pos != string::npos && tmp[pos] != '#')
                line = tmp.substr(0, tmp.find_last_not_of(delims) + 1);
        }
    }
    return line;
}

void image_write_ppm(const image<uint32_t>& img, const string& fileName) {
    FILE* outFile = fopen(fileName.c_str(), "w+b");
    if (!outFile)
        throw runtime_error("Unable to open ppm file.");

    try {
        fprintf(outFile, "P6\n");
        fprintf(outFile, "%d %d\n", (int) img.w, (int) img.h);
        fprintf(outFile, "255\n");

        auto src = &img.bits->at(0);

        size_t numPixels = img.bits->size();

        while (numPixels > 0) {
            // src is B G R A, ppm is R G B
            fwrite(((uint8_t*) src) + 2, 1, 1, outFile);
            fwrite(((uint8_t*) src) + 1, 1, 1, outFile);
            fwrite(((uint8_t*) src) + 0, 1, 1, outFile);
            ++src;
            --numPixels;
        }

        fclose(outFile);
    } catch (...) {
        if (outFile)
            fclose(outFile);
        throw;
    }
}

void aspect_correct_dimensions(uint16_t streamWidth, uint16_t streamHeight,
        uint16_t requestedWidth, uint16_t requestedHeight,
        uint16_t& destWidth, uint16_t& destHeight) {
    destWidth = requestedWidth;
    destHeight = requestedHeight;

    // encode size
    if (streamWidth != 0 && streamHeight != 0) {
        uint16_t newEncodeWidth;
        uint16_t newEncodeHeight;

        if (requestedHeight != 0 && requestedWidth != 0) {
            float streamAspectRatio = streamWidth * 1.0f / streamHeight;
            float maxAspectRatio = requestedWidth * 1.0f / requestedHeight;
            float scaleFactor;

            if (maxAspectRatio < streamAspectRatio)
                scaleFactor = requestedWidth * 1.0f / streamWidth;
            else
                scaleFactor = requestedHeight * 1.0f / streamHeight;

            // original code limited us to scaling down only.
            //scaleFactor = min(scaleFactor, 1.0f);

            uint16_t scaledRoundedPixelWidth = (uint16_t) (streamWidth * scaleFactor + 0.5);
            uint16_t scaledRoundedPixelHeight = (uint16_t) (streamHeight * scaleFactor + 0.5);

            uint16_t multipleOfEightWidth = max(scaledRoundedPixelWidth / 8, 1) * 8;
            uint16_t multipleOfEightHeight = max(scaledRoundedPixelHeight / 8, 1) * 8;

            newEncodeWidth = multipleOfEightWidth;
            newEncodeHeight = multipleOfEightHeight;
        } else {
            newEncodeWidth = streamWidth;
            newEncodeHeight = streamHeight;
        }

        if (requestedWidth != newEncodeWidth)
            destWidth = newEncodeWidth;

        if (requestedHeight != newEncodeHeight)
            destHeight = newEncodeHeight;
    }
}
