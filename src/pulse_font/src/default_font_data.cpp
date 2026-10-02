#include "font_internal.h"

#include <iterator>

namespace pulse_font_internal {

namespace {

const unsigned char kDefaultFntData[] = {
    #include "ProggyVector.fnt.h"
};

const unsigned char kDefaultPngData[] = {
    #include "ProggyVector_0.png.h"
};

}

const std::vector<uint8_t>& default_font_fnt_bytes() {
    static const std::vector<uint8_t> fnt(std::begin(kDefaultFntData), std::end(kDefaultFntData));
    return fnt;
}

const std::vector<uint8_t>& default_font_png_bytes() {
    static const std::vector<uint8_t> png(std::begin(kDefaultPngData), std::end(kDefaultPngData));
    return png;
}

}
