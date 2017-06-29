
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "ppm.h"

#include "test_ppm_data.cpp"

using namespace std;

void test_setup() {
    FILE* outFile = fopen("car.ppm", "w+b");
    for (auto v : ppm)
        fwrite(&v, 1, 1, outFile);
    fclose(outFile);
}

void test_destroy() {
    unlink("car.ppm");
}

int main(int argc, char* argv[]) {
    test_setup();

    {
        auto img = image_create_from_ppm("car.ppm");

        assert(img.w == 364);
        assert(img.h == 243);
        assert(img.bits->size() == (img.w * img.h));

        image_write_ppm(img, "out.ppm");

        auto img2 = image_create_from_ppm("out.ppm");

        assert(img2.w == img.w);
        assert(img2.h == img.h);
        assert(img2.bits->size() == img.bits->size());

        unlink("out.ppm");
    }

    {
        auto img = image_create<uint32_t>(640, 480);

        assert(img.w == 640);
        assert(img.h == 480);
        assert(img.bits->size() == img.w * img.h);
    }

    {
        auto img = image_create<uint32_t>(640, 480);

        assert(img.w == 640);
        assert(img.h == 480);
        assert(img.bits->size() == img.w * img.h);
    }

    {
        auto img = image_create_from_ppm("car.ppm");
        auto lum = image_argb_to_lum<uint8_t>(img);
        auto rgb = image_lum_to_argb<uint8_t>(lum);
        //        image_write_ppm(rgb,"grey.ppm");
        assert(rgb.w == img.w);
        assert(rgb.h == img.h);
    }

    {
        auto img = image_create_from_ppm("car.ppm");
        auto lum = image_argb_to_lum<double>(img);
        auto rgb = image_lum_to_argb<double>(lum);
        //        image_write_ppm(rgb,"greyf.ppm");
        assert(rgb.w == img.w);
        assert(rgb.h == img.h);
    }

    {
        auto img = image_create_from_ppm("car.ppm");
        auto lum = image_argb_to_lum<double>(img);
        auto norm = image_normalize(lum);
        auto rgb = image_lum_to_argb<double>(norm);
        //image_write_ppm(rgb,"norm_grey.ppm");
        auto rgb2 = image_lum_to_argb<double>(lum);
        //image_write_ppm(rgb2,"grey.ppm");
        assert(rgb.w == img.w);
        assert(rgb.h == img.h);
    }

    {
        auto img = image_create<uint32_t>(640, 480);
        // remember, draw rect just draws the lines (it doesn't fill).
        image_draw_rect<uint32_t>(img, 10, 10, 20, 20, 0x80808080);
        auto lum = image_argb_to_lum<double>(img);
        auto ii = image_integral(lum);
        auto sum = image_integral_rectangle(ii, 0, 0, 30, 30);
        // A 10 by 10 line rect has 36 pixels (10 across top, 16 down sides, 10 across bottom)
        assert(sum == 36 * 128);
    }

    {
        auto img = image_create<uint32_t>(640, 480);
        // remember, draw rect just draws the lines (it doesn't fill).
        image_draw_rect<uint32_t>(img, 10, 10, 20, 20, 0x80808080);
        auto lum = image_argb_to_lum<double>(img);
        auto ii = image_squared_integral(lum);
        auto sum = image_integral_rectangle(ii, 0, 0, 30, 30);
        // A 10 by 10 line rect has 36 pixels (10 across top, 16 down sides, 10 across bottom)
        assert(sum == 36 * 16384);
    }

    {
        auto img = image_create_from_ppm("car.ppm");
        uint16_t acw, ach;
        aspect_correct_dimensions(img.w, img.h, 1920, 1080, acw, ach);
        auto scaled = image_resize(img, acw, ach);
        assert(scaled.w == 1616);
        assert(scaled.h == 1080);
        //image_write_ppm( scaled, "scaled.ppm");
    }

    {
        auto img = image_create<uint32_t>(640, 480);

        for (int i = 0; i < 1000; ++i) {
            uint16_t x1 = random() % 638;
            uint16_t y1 = random() % 478;
            uint16_t w = random() % (639 - x1);
            uint16_t h = random() % (479 - y1);
            image_draw_rect(img, x1, y1, x1 + w, y1 + h, (uint32_t) random());
        }
        //image_write_ppm( img, "rects.ppm");
    }

    {
        auto img = image_create_from_ppm("car.ppm");
        auto r90 = image_rotate_90(img);
        image_write_ppm(r90, "r90.ppm");
        auto r180 = image_rotate_180(img);
        image_write_ppm(r180, "r180.ppm");
        auto r270 = image_rotate_270(img);
        image_write_ppm(r270, "r270.ppm");
        auto vmirror = image_mirror_vertical(img);
        image_write_ppm(vmirror, "vmirror.ppm");
    }

    {
        auto img = image_create<uint32_t>(640, 480);

        for (int i = 0; i < 1000; ++i) {
            uint16_t x1 = random() % 638;
            uint16_t y1 = random() % 478;
            uint16_t w = random() % (639 - x1);
            uint16_t h = random() % (479 - y1);
            image_draw_rect(img, x1, y1, x1 + w, y1 + h, (uint32_t) random());
        }

        auto img2 = image_create<uint32_t>(640, 480);
        image_blit(img, 0, 0, 320, 240, img2, 0, 0);
        //image_write_ppm( img2, "blit.ppm" );
    }

    test_destroy();
}
