#ifndef __ppm_h
#define __ppm_h

#include "zip.h"
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <arpa/inet.h>
#include <stdexcept>
#include <numeric>
#include <cmath>

template<class T>
struct image {
    std::shared_ptr<std::vector<T>> bits;
    uint16_t w;
    uint16_t h;
};

template<typename T>
image<T> image_create(uint16_t w, uint16_t h) {
    image<T> img;
    img.bits = std::make_shared<std::vector < T >> (w * h);
    img.w = w;
    img.h = h;
    return img;
}

// Load ppm image from disk into ARGB buffer
image<uint32_t> image_create_from_ppm(const std::string& fileName);

void image_write_ppm(const image<uint32_t>& img, const std::string& fileName);

template<typename T>
void image_blit(const image<T> srcImage, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, image<T> dstImage, uint16_t dx, uint16_t dy) {
    if (sx + width > srcImage.w)
        throw std::runtime_error("geometry exception (sx + width > srcImage.w)");
    if (sy + height > srcImage.h)
        throw std::runtime_error("geometry exception (sy + height > srcImage.h)");
    if (dx + width > dstImage.w)
        throw std::runtime_error("geometry exception (dx + width > dstImage.w)");
    if (dy + height > dstImage.h)
        throw std::runtime_error("geometry exception (dy + height > dstImage.h)");

    const std::vector<T>& src = *srcImage.bits;
    std::vector<T>& dst = *dstImage.bits;

    for (uint16_t h = 0; h < height; ++h) {
        for (uint16_t w = 0; w < width; ++w) {
            dst[ (dx + w) + ((dy + h) * dstImage.w) ] = src[ (sx + w) + ((sy + h) * srcImage.w)];
        }
    }
}

void aspect_correct_dimensions(uint16_t streamWidth, uint16_t streamHeight,
        uint16_t requestedWidth, uint16_t requestedHeight,
        uint16_t& destWidth, uint16_t& destHeight);

template<typename T>
image<T> image_argb_to_lum(const image<uint32_t>& rgb) {
    image<T> out;
    out.w = rgb.w;
    out.h = rgb.h;
    out.bits = std::make_shared<std::vector < T >> (rgb.w * rgb.h);

    zip_transform([](T out, uint32_t in) {
        double b = in >> 24;
        double g = (in >> 16) & 0xFF;
        double r = (in >> 8) & 0xFF;
        return (T) (0.2126f * r + 0.7152f * g + 0.0722f * b);
    },
    out.bits->begin(), out.bits->end(),
            rgb.bits->begin());

    return out;
}

template<typename T>
image<uint32_t> image_lum_to_argb(const image<T>& lum) {
    image<uint32_t> out;
    out.w = lum.w;
    out.h = lum.h;
    out.bits = std::make_shared<std::vector < uint32_t >> (lum.w * lum.h);

    zip_transform([](uint32_t out, T in) {
        uint32_t word;
        if (std::is_floating_point<T>())
                word = 255 << 24 | ((uint8_t) in) << 16 | ((uint8_t) in) << 8 | ((uint8_t) in);
        else word = 255 << 24 | ((uint8_t) in) << 16 | ((uint8_t) in) << 8 | ((uint8_t) in);
            return word;
        },
    out.bits->begin(), out.bits->end(),
            lum.bits->begin());

    return out;
}

template<typename T>
image<T> image_normalize(const image<T>& input) {
    T mean = std::is_floating_point<T>() ? 0.0 : 0;
    mean = std::accumulate(input.bits->begin(), input.bits->end(),
            mean);
    mean /= input.w * input.h;

    T stdev = std::is_floating_point<T>() ? 0.0 : 0;
    stdev = std::accumulate(input.bits->begin(), input.bits->end(),
            stdev,
            [mean](T accum, T val) {
                return accum + pow(val - mean, 2);
            });
    stdev /= input.w * input.h;
    stdev = sqrt(stdev);

    image<T> out;
    out.w = input.w;
    out.h = input.h;
    out.bits = std::make_shared<std::vector < T >> (input.w * input.h);

    zip_transform([mean, stdev](T out, T in) {
        return (in - mean) / stdev;
    },
    out.bits->begin(), out.bits->end(),
            input.bits->begin());

    return out;
}

template<typename T>
image<T> image_integral(const image<T>& input) {
    image<T> out;
    out.w = input.w;
    out.h = input.h;
    out.bits = std::make_shared<std::vector < T >> (input.w * input.h);

    std::vector<T> s(input.w * input.h);

    auto& img = *input.bits;
    auto& ii = *out.bits;

    for (uint16_t y = 0; y < input.h; ++y) {
        for (uint16_t x = 0; x < input.w; ++x) {
            if (x == 0)
                s[(y * input.w) + x] = img[(y * input.w) + x];
            else s[(y * input.w) + x] = s[(y * input.w) + x - 1] + img[(y * input.w) + x];
            if (y == 0)
                ii[(y * input.w) + x] = s[(y * input.w) + x];
            else ii[(y * input.w) + x] = ii[((y - 1) * input.w) + x] + s[(y * input.w) + x];
        }
    }

    return out;
}

template<typename T>
image<T> image_squared_integral(const image<T>& input) {
    image<T> out;
    out.w = input.w;
    out.h = input.h;
    out.bits = std::make_shared<std::vector < T >> (input.w * input.h);

    std::vector<T> s(input.w * input.h);

    auto& img = *input.bits;
    auto& ii = *out.bits;

    for (uint16_t y = 0; y < input.h; ++y) {
        for (uint16_t x = 0; x < input.w; ++x) {
            if (x == 0)
                s[(y * input.w) + x] = pow(img[(y * input.w) + x], 2);
            else s[(y * input.w) + x] = s[(y * input.w) + x - 1] + pow(img[(y * input.w) + x], 2);
            if (y == 0)
                ii[(y * input.w) + x] = s[(y * input.w) + x];
            else ii[(y * input.w) + x] = ii[((y - 1) * input.w) + x] + s[(y * input.w) + x];
        }
    }

    return out;
}

template<typename T>
T image_integral_rectangle(const image<T>& input, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    auto& ii = *input.bits;
    T value = ii[((y + h - 1) * input.w)+(x + w - 1)];
    if (x > 0) value -= ii[((y + h - 1) * input.w)+(x - 1)];
    if (y > 0) value -= ii[(y - 1) * input.w + (x + w - 1)];
    if (x > 0 && y > 0) value += ii[(y - 1) * input.w + (x - 1)];
    return value;
}

template<typename T>
image<T> image_rotate_90(const image<T>& input) {
    image<T> out;
    out.w = input.h;
    out.h = input.w;
    out.bits = std::make_shared<std::vector < T >> (input.w * input.h);

    std::vector<T>& img = *out.bits;
    const std::vector<T>& src = *input.bits;

    for (uint16_t y = 0; y < input.h; ++y) {
        for (uint16_t x = 0; x < input.w; ++x) {
            img[input.h - 1 - y + (x * input.h)] = src[(y * input.w) + x];
        }
    }

    return out;
}

template<typename T>
image<T> image_rotate_180(const image<T>& input) {
    image<T> out;
    out.w = input.w;
    out.h = input.h;
    out.bits = std::make_shared<std::vector < T >> (input.w * input.h);

    std::vector<T>& img = *out.bits;
    const std::vector<T>& src = *input.bits;

    for (uint16_t y = 0; y < input.h; ++y) {
        for (uint16_t x = 0; x < input.w; ++x) {
            img[((input.h - 1 - y) * input.w) + input.w - 1 - x] = src[(y * input.w) + x];
        }
    }

    return out;
}

template<typename T>
image<T> image_rotate_270(const image<T>& input) {
    image<T> out;
    out.w = input.h;
    out.h = input.w;
    out.bits = std::make_shared<std::vector < T >> (input.w * input.h);

    std::vector<T>& img = *out.bits;
    const std::vector<T>& src = *input.bits;

    for (uint16_t y = 0; y < input.h; ++y) {
        for (uint16_t x = 0; x < input.w; ++x) {
            img[y + ((input.w - x - 1) * input.h)] = src[(y * input.w) + x];
        }
    }

    return out;
}

template<typename T>
image<T> image_mirror_vertical(const image<T>& input) {
    image<T> out;
    out.w = input.w;
    out.h = input.h;
    out.bits = std::make_shared<std::vector < T >> (input.w * input.h);

    std::vector<T>& img = *out.bits;
    const std::vector<T>& src = *input.bits;

    for (uint16_t y = 0; y < input.h; ++y) {
        for (uint16_t x = 0; x < input.w; ++x) {
            img[(((input.h - 1) - y) * input.w) + x] = src[(y * input.w) + x];
        }
    }

    return out;
}

template<typename T>
image<T> image_resize(const image<T>& input, uint16_t outputWidth, uint16_t outputHeight) {
    image<T> out;
    out.w = outputWidth;
    out.h = outputHeight;
    out.bits = std::make_shared<std::vector < T >> (outputWidth * outputHeight);
    T* dst = &out.bits->at(0);
    const T* src = &input.bits->at(0);

    int a, b, c, d, x, y, index;
    float x_ratio = ((float) (input.w - 1)) / outputWidth;
    float y_ratio = ((float) (input.h - 1)) / outputHeight;
    float x_diff, y_diff, blue, red, green;
    int offset = 0;

    for (int i = 0; i < outputHeight; i++) {
        for (int j = 0; j < outputWidth; j++) {
            x = (int) (x_ratio * j);
            y = (int) (y_ratio * i);
            x_diff = (x_ratio * j) - x;
            y_diff = (y_ratio * i) - y;
            index = (y * input.w + x);
            a = src[index];
            b = src[index + 1];
            c = src[index + input.w];
            d = src[index + input.w + 1];

            // blue element
            blue = (a & 0xff)*(1 - x_diff)*(1 - y_diff) + (b & 0xff)*(x_diff)*(1 - y_diff) +
                    (c & 0xff)*(y_diff)*(1 - x_diff) + (d & 0xff)*(x_diff * y_diff);

            // green element
            green = ((a >> 8)&0xff)*(1 - x_diff)*(1 - y_diff) + ((b >> 8)&0xff)*(x_diff)*(1 - y_diff) +
                    ((c >> 8)&0xff)*(y_diff)*(1 - x_diff) + ((d >> 8)&0xff)*(x_diff * y_diff);

            // red element
            red = ((a >> 16)&0xff)*(1 - x_diff)*(1 - y_diff) + ((b >> 16)&0xff)*(x_diff)*(1 - y_diff) +
                    ((c >> 16)&0xff)*(y_diff)*(1 - x_diff) + ((d >> 16)&0xff)*(x_diff * y_diff);

            dst [offset++] = 0xff << 24 |
                    ((int) red) << 16 |
                    ((int) green) << 8 |
                    ((int) blue);
        }
    }

    return out;
}

template<typename T>
void image_draw_rect(image<T>& input, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, T color) {
    uint16_t w = x2 - x1;
    uint16_t h = y2 - y1;

    T* dst = &input.bits->at(0) + x1 + (y1 * input.w);

    // top line
    for (uint16_t i = 0; i < w; ++i)
        *dst++ = color;

    if (h > 2) {
        for (uint16_t i = 0; i < (h - 2); ++i) {
            dst += (input.w - w);
            *dst = color;
            dst += w;
            *dst = color;
        }
    }

    dst += (input.w - w);

    for (uint16_t i = 0; i < w; ++i)
        *dst++ = color;
}

#endif
