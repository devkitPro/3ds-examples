/*------------------------------------------------------------------------------
 * Copyright (c) 2024
 *     ds-sloth
 *
 * This file is part of tex3ds.
 *
 * tex3ds is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tex3ds is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tex3ds.  If not, see <http://www.gnu.org/licenses/>.
 *----------------------------------------------------------------------------*/
/** @file tex3ds.h
 *  @brief Tex3DS
 */

#pragma once

#include <cstdint>
#include <cstddef>

#include <vector>
#include <string>

namespace rg_etc1
{

// Quality setting = the higher the quality, the slower.
// To pack large textures, it is highly recommended to call pack_etc1_block() in parallel, on different blocks, from multiple threads (particularly when using cHighQuality).
enum etc1_quality
{
   cLowQuality,
   cMediumQuality,
   cHighQuality,
};

} // namespace rg_etc1


namespace Tex3DS
{

using Quantum = uint8_t;
static constexpr uint8_t QuantumRange = 255;

struct RGBA
{
    Quantum b, g, r, a;
};

using PixelPacket = RGBA*;

struct Image
{
    size_t stride = 0;
    size_t w = 0;
    size_t h = 0;
    std::vector<RGBA> pixels;

    Image() = default;
    Image(size_t _w, size_t _h)
    {
        w = _w;
        h = _h;
        stride = w;
        pixels.resize(w * h);
    }

};

/** @brief Process format */
enum ProcessFormat
{
    RGBA8888 = 0x00, ///< RGBA8888 encoding
    RGB888   = 0x01, ///< RGB888 encoding
    RGBA5551 = 0x02, ///< RGBA5551 encoding
    RGB565   = 0x03, ///< RGB565 encoding
    RGBA4444 = 0x04, ///< RGBA4444 encoding
    LA88     = 0x05, ///< LA88 encoding
    HILO88   = 0x06, ///< HILO88 encoding
    L8       = 0x07, ///< L8 encoding
    A8       = 0x08, ///< A8 encoding
    LA44     = 0x09, ///< LA44 encoding
    L4       = 0x0A, ///< L4 encoding
    A4       = 0x0B, ///< A4 encoding
    ETC1     = 0x0C, ///< ETC1 encoding
    ETC1A4   = 0x0D, ///< ETC1A4 encoding
    AUTO_L8,         ///< L8/LA88 encoding
    AUTO_L4,         ///< L4/LA44 encoding
    AUTO_ETC1,       ///< ETC1/ETC1A4 encoding
};

/** @brief Compression format */
enum CompressionFormat
{
    COMPRESSION_NONE, ///< No compression
    COMPRESSION_LZ10, ///< LZSS/LZ10 compression
    COMPRESSION_LZ11, ///< LZ11 compression
    COMPRESSION_RLE,  ///< Run-length encoding compression
    COMPRESSION_HUFF, ///< Huffman encoding
    COMPRESSION_AUTO, ///< Choose best compression
};

struct Params
{
    /** @brief Output path option */
    std::string output_path;

    /** @brief Process format option */
    ProcessFormat process_format = RGBA8888;

    /** @brief ETC1 quality option */
    rg_etc1::etc1_quality etc1_quality = rg_etc1::cMediumQuality;

    /** @brief Compression format option */
    CompressionFormat compression_format = COMPRESSION_AUTO;

    /** @brief Input image */
    Image input_img;
};

bool Process(const Params& params);

} // namespace Tex3DS
