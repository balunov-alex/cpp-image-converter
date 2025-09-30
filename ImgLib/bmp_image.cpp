#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

static const string_view BPM_SIG = "BM"sv;
static const uint32_t BPM_RESERVED_SPACE = 0;
static const uint16_t BPM_PLANES_COUNT = 1;
static const uint16_t BPM_BITS_PER_PIXEL = 24;
static const uint32_t BPM_COMPRESSION_TYPE = 0;
static const int32_t BPM_HOR_RESOLUTION = 11811;
static const int32_t BPM_VER_RESOLUTION = 11811;
static const int32_t BPM_USED_COLORS_COUNT = 0;
static const int32_t BPM_SAGNIFICANT_COLORS_COUNT = 0x1000000;
static const uint32_t BPM_DATA_START_POS = 54;
static const uint32_t BPM_INFO_HEADER_SIZE = 40;

PACKED_STRUCT_BEGIN BitmapFileHeader {
    char format_char_1;
    char format_char_2;
    uint32_t total_size;
    uint32_t reserved_space;
    uint32_t data_start_pos;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t info_header_size;
    int32_t image_width;
    int32_t image_height;
    uint16_t planes_count;
    uint16_t bits_per_pixel;
    uint32_t compression_type;
    uint32_t data_bytes_count;
    int32_t hor_resolution;
    int32_t ver_resolution;
    int32_t used_colors_count;
    int32_t significant_colors_count;
}
PACKED_STRUCT_END

static const int BYTES_PER_PIXEL = 3;

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    const int rounding_up = 3;
    const int alignment = 4;
    return alignment * ((w * BYTES_PER_PIXEL + rounding_up) / alignment);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out.is_open()) {
        return false;
    }

    const int32_t h = image.GetHeight();
    const int32_t w = image.GetWidth();
    const int32_t stride = GetBMPStride(w);
    const uint32_t data_bytes_count = stride * h;
    const uint32_t total_size = BPM_DATA_START_POS + data_bytes_count;

    BitmapFileHeader file_header = {BPM_SIG[0], BPM_SIG[1], total_size, BPM_RESERVED_SPACE, BPM_DATA_START_POS};
    BitmapInfoHeader info_header = {BPM_INFO_HEADER_SIZE, w, h, BPM_PLANES_COUNT, BPM_BITS_PER_PIXEL, 
                                    BPM_COMPRESSION_TYPE, data_bytes_count, BPM_HOR_RESOLUTION, BPM_VER_RESOLUTION, 
                                    BPM_USED_COLORS_COUNT, BPM_SAGNIFICANT_COLORS_COUNT};

    if (!out.write(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader))) {
        return false;
    }
    if (!out.write(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader))) {
        return false;
    }

    std::vector<char> buff(stride);
    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            buff[x * BYTES_PER_PIXEL + 0] = static_cast<char>(line[x].b);
            buff[x * BYTES_PER_PIXEL + 1] = static_cast<char>(line[x].g);
            buff[x * BYTES_PER_PIXEL + 2] = static_cast<char>(line[x].r);
        }
        if (!out.write(buff.data(), stride)) {
            return false;
        }
    }
    return true;
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    if (!ifs.is_open()) {
        return {};
    }

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;
      
    if (!ifs.read(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader))) {
        return {};
    }
    if (!ifs.read(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader))) {
        return {};
    }
    
    const int32_t w = info_header.image_width;
    const int32_t h = info_header.image_height;
    const int32_t stride = GetBMPStride(w);
    const uint32_t data_bytes_count = stride * h;
    const uint32_t total_size = file_header.data_start_pos + data_bytes_count;

    if (file_header.format_char_1 != BPM_SIG[0]
        || file_header.format_char_2 != BPM_SIG[1]
        || file_header.total_size != total_size
        || file_header.reserved_space != BPM_RESERVED_SPACE
        || file_header.data_start_pos != BPM_DATA_START_POS
        || info_header.info_header_size != BPM_INFO_HEADER_SIZE
        || info_header.planes_count != BPM_PLANES_COUNT
        || info_header.bits_per_pixel != BPM_BITS_PER_PIXEL
        || info_header.compression_type != BPM_COMPRESSION_TYPE
        || info_header.data_bytes_count != data_bytes_count
        || info_header.hor_resolution != BPM_HOR_RESOLUTION
        || info_header.ver_resolution != BPM_VER_RESOLUTION
        || info_header.used_colors_count != BPM_USED_COLORS_COUNT
        || info_header.significant_colors_count != BPM_SAGNIFICANT_COLORS_COUNT) 
    {
        return {};
    }
    
    Image result(w, h, Color::Black());
    std::vector<char> buff(stride);

    for (int y = h - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        if (!ifs.read(buff.data(), stride)) {
            return {};
        }
        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buff[x * BYTES_PER_PIXEL + 0]);
            line[x].g = static_cast<byte>(buff[x * BYTES_PER_PIXEL + 1]);
            line[x].r = static_cast<byte>(buff[x * BYTES_PER_PIXEL + 2]);
        }
    }

    return result;
}

}  // namespace img_lib
