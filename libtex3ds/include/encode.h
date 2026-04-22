/*------------------------------------------------------------------------------
 * Copyright (c) 2017-2019
 *     Michael Theall (mtheall)
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
/** @file encode.h
 *  @brief Image encoding routines
 *
 *  @note All of the encoding routines expect the tile to be swizzled, except
 *  for ETC1/ETC1A4 which do not involve swizzling.
 */
#pragma once

#include "libtex3ds.h"
#include "rg_etc1.h"
#include "subimage.h"

#include <limits>
#include <vector>

namespace Tex3DS
{

/** @namespace encode
 *  @brief Image encoding namespace
 */
namespace encode
{
/** @brief Data buffer */
typedef std::vector<uint8_t> Buffer;

/** @brief Encode data
 *  @tparam     T   Type to encode
 *  @param[in]  in  Data to encode
 *  @param[out] out Output buffer
 */
template <typename T>
inline void encode (const T &in, Buffer &out);

/** @brief Encode uint8_t
 *  @param[in]  in  Data to encode
 *  @param[out] out Output buffer
 */
template <>
inline void encode<uint8_t> (const uint8_t &in, Buffer &out)
{
	out.push_back (in);
}

/** @brief Encode uint16_t
 *  @param[in]  in  Data to encode
 *  @param[out] out Output buffer
 */
template <>
inline void encode<uint16_t> (const uint16_t &in, Buffer &out)
{
	out.push_back (in >> 0);
	out.push_back (in >> 8);
}

/** @brief Encode uint32_t
 *  @param[in]  in  Data to encode
 *  @param[out] out Output buffer
 */
template <>
inline void encode<uint32_t> (const uint32_t &in, Buffer &out)
{
	out.push_back (in >> 0);
	out.push_back (in >> 8);
	out.push_back (in >> 16);
	out.push_back (in >> 24);
}

/** @brief Encode float
 *  @param[in]  in  Data to encode
 *  @param[out] out Output buffer
 */
template <>
inline void encode<float> (const float &in, Buffer &out)
{
	constexpr uint16_t scale = 1024;

	assert (in >= 0);
	assert (in <= std::numeric_limits<uint16_t>::max () / scale);

	encode<uint16_t> (in * scale, out);
}

/** @brief Encode sub-image
 *  @param[in]  sub    Sub-image to encode
 *  @param[in]  width  Sub-image width (pixels)
 *  @param[in]  height Sub-image height (pixels)
 *  @param[out] out    Output buffer
 */
inline void encode (const SubImage &sub, uint16_t width, uint16_t height, Buffer &out)
{
	encode<uint16_t> (width, out);
	encode<uint16_t> (height, out);

	encode<float> (sub.left, out);
	encode<float> (sub.top, out);
	encode<float> (sub.right, out);
	encode<float> (sub.bottom, out);
}

/** @brief Work unit
 *
 *  @details
 *  A work unit encapsulates the work needed to process a single 8x8 tile from
 *  a texture.
 */
struct WorkUnit
{
	Buffer result;                      ///< Work result
	uint64_t sequence;                  ///< Work identifier
	PixelPacket p;                      ///< Pixel data buffer
	size_t stride;                      ///< Pixel data stride
	rg_etc1::etc1_quality etc1_quality; ///< ETC1 quality option
	bool output;                        ///< Whether to output 3DS data
	bool preview;                       ///< Whether to output preview image
	void (*process) (WorkUnit &);       ///< Work unit processor

	/** @brief Constructor
	 *  @param[in] sequence     Work identifier
	 *  @param[in] p            Pixel data buffer
	 *  @param[in] stride       Pixel data stride
	 *  @param[in] etc1_quality ETC1 quality option
	 *  @param[in] output       Whether to output 3DS data
	 *  @param[in] preview      Whether to output preview image
	 *  @param[in] process      Work unit processor
	 */
	WorkUnit (uint64_t sequence,
	    PixelPacket p,
	    size_t stride,
	    rg_etc1::etc1_quality etc1_quality,
	    bool output,
	    bool preview,
	    void (*process) (WorkUnit &))
	    : sequence (sequence),
	      p (p),
	      stride (stride),
	      etc1_quality (etc1_quality),
	      output (output),
	      preview (preview),
	      process (process)
	{
	}

	WorkUnit ()                      = delete;
	WorkUnit (const WorkUnit &other) = delete;
	WorkUnit (WorkUnit &&other)      = default;
	WorkUnit &operator= (const WorkUnit &other) = delete;
	WorkUnit &operator= (WorkUnit &&other) = default;

	/** @brief Work unit comparator
	 *  @param[in] other Work unit to compare
	 *  @returns sequence > other.sequence
	 */
	bool operator< (const WorkUnit &other) const
	{
		/* greater-than for min-heap */
		return sequence > other.sequence;
	}
};

/** @brief RGBA8888 encoder
 *
 *  @details
 *  Outputs the tile in RGBA8888 (32bpp) format. Data is output in ABGR order.
 *
 *  If the source image has no alpha channel, every pixel will be opaque.
 *
 *  @param[in] work Work unit
 */
void rgba8888 (WorkUnit &work);

/** @brief RGB888 encoder
 *
 *  @details
 *  Outputs the tile in RGB888 (24bpp) format. Data is output in BGR order. The
 *  alpha channel is ignored; every pixel is opaque.
 *
 *  @param[in] work Work unit
 */
void rgb888 (WorkUnit &work);

/** @brief RGB565 encoder
 *
 *  @details
 *  Outputs the tile in RGB565 (16bpp) format. The upper 5 bits are red, the
 *  middle 6 bits are green, the lower 5 bits are blue. The data is output as
 *  16-bit values in little-endian. The alpha channel is ignored; every pixel is
 *  opaque.
 *
 *  @param[in] work Work unit
 */
void rgb565 (WorkUnit &work);

/** @brief RGBA5551 encoder
 *
 *  @details
 *  Outputs the tile in RGBA5551 (16bpp) format. The upper 5 bits are red, the
 *  next lower 5 bits are green, the next lower 5 bits are blue, the lowest bit
 *  is alpha. The data is output as 16-bit values in little-endian.
 *
 *  If the source image has no alpha channel, every pixel will be opaque.
 *
 *  @param[in] work Work unit
 */
void rgba5551 (WorkUnit &work);

/** @brief RGBA4444 encoder
 *
 *  @details
 *  Outputs the tile in RGBA4444 (16bpp) format. The upper 4 bits are red, the
 *  next lower 4 bits are green, the next lower 4 bits are blue, the lower 4
 *  bits are alpha. The data is output as 16-bit values in little-endian.
 *
 *  If the source image has no alpha channel, every pixel will be opaque.
 *
 *  @param[in] work Work unit
 */
void rgba4444 (WorkUnit &work);

/** @brief LA88 encoder
 *
 *  @details
 *  Outputs the tile in LA88 (16bpp) format. L is the luminance, which is
 *  calculated from the RGB components with gamma correction. The data is output
 *  in AL order.
 *
 *  If the source image has no alpha channel, every pixel will be opaque.
 *
 *  @param[in] work Work unit
 */
void la88 (WorkUnit &work);

/** @brief HILO88 encoder
 *
 *  @details
 *  Outputs the tile in HILO88 (16bpp) format. HI corresponds to the data in the
 *  red channel; LO corresponds to the data in the green channel. The blue and
 *  alpha channels are ignored. The data is output in LOHI order.
 *
 *  @param[in] work Work unit
 */
void hilo88 (WorkUnit &work);

/** @brief L8 encoder
 *
 *  @details
 *  Outputs the tile in L8 (8bpp) format. L is the luminance, which is
 *  calculated from the RGB components with gamma correction. The alpha channel
 *  is ignored; every pixel is opaque.
 *
 *  @param[in] work Work unit
 */
void l8 (WorkUnit &work);

/** @brief A8 encoder
 *
 *  @details
 *  Outputs the tile in A8 (8bpp) format. The RGB channels are ignored.
 *
 *  If the source image has no alpha channel, every pixel will be opaque.
 *
 *  @param[in] work Work unit
 */
void a8 (WorkUnit &work);

/** @brief LA44 encoder
 *
 *  @details
 *  Outputs the tile in LA44 (8bpp) format. L is the luminance, which is
 *  calculated from the RGB components with gamma correction. The upper 4 bits
 *  are luminance, the lower 4 bits are alpha.
 *
 *  If the source image has no alpha channel, every pixel will be opaque.
 *
 *  @param[in] work Work unit
 */
void la44 (WorkUnit &work);

/** @brief L4 encoder
 *
 *  @details
 *  Outputs the tile in L4 (4bpp) format. L is the luminance, which is
 *  calculated from the RGB components with gamma correction. For each pair of
 *  pixels, the first resides in the lower 4 bits, and the second resides in the
 *  upper 4 bits. The alpha channel is ignored; every pixel is opaque.
 *
 *  @param[in] work Work unit
 */
void l4 (WorkUnit &work);

/** @brief A4 encoder
 *
 *  @details
 *  Outputs the tile in A8 (8bpp) format. The RGB channels are ignored. For each
 *  pair of pixels, the first resides in the lower 4 bits, and the second
 *  resides in the upper 4 bits.
 *
 *  If the source image has no alpha channel, every pixel will be opaque.
 *
 *  @param[in] work Work unit
 */
void a4 (WorkUnit &work);

/** @brief ETC1 encoder
 *
 *  @details
 *  Outputs the tile in ETC1 (4bpp) format. The tile is split into 4 4x4 blocks,
 *  each of which are encoded into a 64-bit value. Each 64-bit value is output
 *  in little-endian order. The alpha channel is ignored; every pixel is opaque.
 *
 *  @param[in] work Work unit
 */
void etc1 (WorkUnit &work);

/** @brief ETC1A4 encoder
 *
 *  @details
 *  Outputs the tile in ETC1A4 (8bpp) format. The tile is split into 4 4x4
 *  blocks, each of which are encoded into an 8-byte alpha block followed by the
 *  64-bit ETC1 encoding. The alpha block is encoded with X/Y transposed. For
 *  each pair of pixels, the first resides in the lower 4 bits, and the second
 *  resides in the upper 4 bits. Each 64-bit ETC1 value is output in
 *  little-endian order. The alpha channel is ignored; every pixel is opaque.
 *
 *  @param[in] work Work unit
 */
void etc1a4 (WorkUnit &work);
}

} // namespace Tex3DS
