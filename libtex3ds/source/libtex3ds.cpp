/*------------------------------------------------------------------------------
 * Copyright (c) 2017-2022
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
/** @file tex3ds.cpp
 *  @brief tex3ds program entry point
 */

#include "compress.h"
#include "encode.h"
#include "libtex3ds.h"
#include "quantum.h"
#include "rg_etc1.h"
#include "subimage.h"
#include "swizzle.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <set>
#include <vector>

namespace Tex3DS
{
/** @brief Get power-of-2 ceiling
 *  @param[in] x Value to calculate
 *  @returns Power-of-2 ceiling
 */
static inline size_t potCeil (size_t x)
{
	if (x < 8)
		return 8;

	return std::pow (2.0, std::ceil (std::log2 (x)));
}

/** @brief Processing mode */
enum ProcessingMode
{
	PROCESS_NORMAL,  ///< Normal
};

struct Invocation
{
	const Params& params;

	Invocation(const Params& p) : params(p) {}

	/** @brief Output raw image data */
	bool output_raw = false;

	/** @brief Output subimage data */
	std::vector<SubImage> subimage_data;

	/** @brief Output image data */
	encode::Buffer image_data;

	/** @brief Output width */
	size_t output_width;

	/** @brief Output height */
	size_t output_height;

	Tex3DS::Image load_image (const Tex3DS::Image &img);
	void process_image (Tex3DS::Image &img);
	void write_tex3ds_header (FILE *fp);
	void write_image_data (FILE *fp);
	void write_output_data();

	int go();
};

/** @brief Processing mode option */
static constexpr ProcessingMode process_mode = PROCESS_NORMAL;

/** @brief Maximum output height */
static constexpr size_t max_image_height = 1024;

/** @brief Maximum output width */
static constexpr size_t max_image_width = 1024;

/** @brief Load image
 *  @param[in] img Input image
 *  @returns vector of images to process
 */
Tex3DS::Image Invocation::load_image (const Tex3DS::Image &img)
{
	double width  = img.w;
	double height = img.h;

	// check for valid width
	if (width > max_image_width)
	{
		std::fprintf (stderr, "Invalid width\n");
		return img;
	}

	// check for valid height
	if (height > max_image_height)
	{
		std::fprintf (stderr, "Invalid height\n");
		return img;
	}

	// apply border/edge
	output_width  = potCeil (img.w);
	output_height = potCeil (img.h);

	size_t image_width  = img.w;
	size_t image_height = img.h;

	if (process_mode == PROCESS_NORMAL)
	{
		assert (subimage_data.empty ());
		subimage_data.emplace_back (0,
		    "",
		    0.0f,
		    1.0f,
		    static_cast<float> (image_width) / output_width,
		    1.0f - static_cast<float> (image_height) / output_height,
		    false);
	}

	if (image_width != output_width || image_height != output_height)
	{
		// expand canvas
		Tex3DS::Image new_img = Tex3DS::Image(output_width, output_height);

		for(size_t r = 0; r < image_height; r++)
			memcpy(&new_img.pixels[new_img.stride * r], &img.pixels[img.stride * r], img.stride * sizeof(Tex3DS::RGBA));

		return new_img;
	}

	return img;
}

/** @brief Process image
 *  @param[in] img Image to process
 */
void Invocation::process_image (Tex3DS::Image &img)
{
	void (*process) (encode::WorkUnit &) = nullptr;

	// get the processing routine
	switch (params.process_format)
	{
	case RGBA8888:
		process = encode::rgba8888;
		break;

	case RGB888:
		process = encode::rgb888;
		break;

	case RGBA5551:
		process = encode::rgba5551;
		break;

	case RGB565:
		process = encode::rgb565;
		break;

	case RGBA4444:
		process = encode::rgba4444;
		break;

	case AUTO_L8:
	case LA88:
		process = encode::la88;
		break;

	case HILO88:
		process = encode::hilo88;
		break;

	case L8:
		process = encode::l8;
		break;

	case A8:
		process = encode::a8;
		break;

	case AUTO_L4:
	case LA44:
		process = encode::la44;
		break;

	case L4:
		process = encode::l4;
		break;

	case A4:
		process = encode::a4;
		break;

	case ETC1:
		process = encode::etc1;
		break;

	case AUTO_ETC1:
	case ETC1A4:
		process = encode::etc1a4;
		break;

	}

	// get the mipmap dimensions
	size_t width  = img.w;
	size_t height = img.h;

	assert (width % 8 == 0);
	assert (height % 8 == 0);

	// all formats are swizzled except ETC1/ETC1A4
	if (params.process_format != ETC1 && params.process_format != ETC1A4)
		swizzle (img, false);

	// process each 8x8 tile
	uint64_t num_work = 0;
	for (size_t j = 0; j < height; j += 8)
	{
		for (size_t i = 0; i < width; i += 8)
		{
			// create the work unit
			encode::WorkUnit work (num_work++,
			    img.pixels.data() + (j * img.stride + i),
			    img.stride,
			    params.etc1_quality,
			    !params.output_path.empty (),
			    false,
			    process);

			work.process (work);

			// append the result's output buffer
			image_data.insert (image_data.end (), work.result.begin (), work.result.end ());
		}
	}
}

/** @brief Write buffer
 *  @param[in] fp File handle
 */
void write_buffer (FILE *fp, const void *buffer, size_t size)
{
	const uint8_t *buf = static_cast<const uint8_t *> (buffer);
	size_t pos         = 0;

	while (pos < size)
	{
		size_t rc = std::fwrite (buf + pos, 1, size - pos, fp);
		if (rc == 0)
		{
			std::fclose (fp);
			return;
		}

		pos += rc;
	}
}

/** @brief Write Tex3DS header
 *  @param[in] fp File handle
 */
void Invocation::write_tex3ds_header (FILE *fp)
{
	encode::Buffer buf;

	encode::encode<uint16_t> (subimage_data.size (), buf);

	uint8_t texture_params = 0;

	assert (output_width >= 8);
	assert (output_width <= 1024);
	assert (output_height >= 8);
	assert (output_height <= 1024);

	uint8_t w = std::log (static_cast<double> (output_width)) / std::log (2.0);
	uint8_t h = std::log (static_cast<double> (output_height)) / std::log (2.0);

	assert (w >= 3);
	assert (w <= 10);
	assert (h >= 3);
	assert (h <= 10);

	texture_params |= (w - 3) << 0;
	texture_params |= (h - 3) << 3;

	encode::encode<uint8_t> (texture_params, buf);
	encode::encode<uint8_t> (params.process_format, buf);

	uint8_t num_mipmaps = 0;
	encode::encode<uint8_t> (num_mipmaps, buf);

	// encode subimage info
	for (const auto &sub : subimage_data)
	{
		uint16_t width;
		uint16_t height;

#ifndef NDEBUG
		sub.print (output_width, output_height);
#endif

		if (sub.rotated)
		{
			height = (sub.bottom - sub.top) * output_width;
			width  = (sub.right - sub.left) * output_height;
		}
		else
		{
			width  = (sub.right - sub.left) * output_width;
			height = (sub.top - sub.bottom) * output_height;
		}

		encode::encode (sub, width, height, buf);
	}

	write_buffer (fp, buf.data (), buf.size ());
}

/** @brief Dummy compression
 *  @param[in]  src    Source buffer
 *  @param[in]  len    Source length
 *  @returns "Compressed" buffer
 */
std::vector<uint8_t> compressNone (const void *src, size_t len)
{
	const uint8_t *source = reinterpret_cast<const uint8_t *> (src);

	std::vector<uint8_t> result;

	// append compression header
	compressionHeader (result, 0x00, len);

	// add data
	result.insert (std::end (result), source, source + len);

	// pad the output buffer to 4 bytes
	if (result.size () & 0x3)
		result.resize ((result.size () + 3) & ~0x3);

	return result;
}

/** @brief Auto-select compression
 *  @param[in]  src    Source buffer
 *  @param[in]  len    Source length
 *  @returns Compressed buffer
 */
std::vector<uint8_t> compressAuto (const void *src, size_t len)
{
	std::vector<uint8_t> best;

	static std::pair<std::vector<uint8_t> (*) (const void *, size_t), const char *>
	    compress_funcs[] = {
	        {&compressNone, "none"},
	        {&lzssEncode, "lzss"},
	        {&lz11Encode, "lz11"},
	        {&huffEncode, "huff"},
	        {&rleEncode, "rle"},
	    };

	const char *best_type = nullptr;

	for (const auto &compress : compress_funcs)
	{
		std::vector<uint8_t> output = compress.first (src, len);

		if (best.empty () || (!output.empty () && output.size () < best.size ()))
		{
			best.swap (output);
			best_type = compress.second;
		}
	}

	std::printf ("Used %s for compression\n", best_type);
	return best;
}

/** @brief Write image data
 *  @param[in] fp File handle
 */
void Invocation::write_image_data (FILE *fp)
{
	std::vector<uint8_t> (*compress) (const void *, size_t) = nullptr;

	// get the compression routine
	switch (params.compression_format)
	{
	case COMPRESSION_NONE:
		compress = &compressNone;
		break;

	case COMPRESSION_LZ10:
		compress = &lzssEncode;
		break;

	case COMPRESSION_LZ11:
		compress = &lz11Encode;
		break;

	case COMPRESSION_RLE:
		compress = &rleEncode;
		break;

	case COMPRESSION_HUFF:
		compress = &huffEncode;
		break;

	case COMPRESSION_AUTO:
		compress = &compressAuto;
		break;

	default:
		// We should only get a valid type here
		std::abort ();
	}

	// compress data
	std::vector<uint8_t> buffer = compress (image_data.data (), image_data.size ());
	if (buffer.empty ())
	{
		std::fclose (fp);
		std::fprintf (stderr, "Failed to compress data\n");
		return;
	}

	// output data
	write_buffer (fp, buffer.data (), buffer.size ());
}

/** @brief Write output data
 */
void Invocation::write_output_data ()
{
	// check if we need to output the data
	if (params.output_path.empty ())
		return;

	FILE *fp = std::fopen (params.output_path.c_str (), "wb");
	if (!fp)
	{
		std::fprintf (stderr, "Failed to open output file\n");
		return;
	}

	if (!output_raw)
		write_tex3ds_header (fp);

	write_image_data (fp);

	// close output file
	std::fclose (fp);
}

int Invocation::go ()
{
	// initialize rg_etc1 if ETC1/ETC1A format chosen
	if (params.process_format == ETC1 || params.process_format == ETC1A4 || params.process_format == AUTO_ETC1)
		rg_etc1::pack_etc1_block_init ();

	Tex3DS::Image img = load_image (params.input_img);
	process_image (img);
	write_output_data ();

	return EXIT_SUCCESS;
}

bool Process(const Params& params)
{
	Invocation i(params);
	return i.go() == EXIT_SUCCESS;
}

} // namespace Tex3DS
