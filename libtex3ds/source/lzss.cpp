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
/** @file lzss.cpp
 *  @brief LZSS/LZ10/LZ11 compression routines
 */

#include "compress.h"

#include <cstring>
#include <vector>

/** @brief LZSS/LZ10 maximum match length */
#define LZ10_MAX_LEN 18

/** @brief LZSS/LZ10 maximum displacement */
#define LZ10_MAX_DISP 4096

/** @brief LZ11 maximum match length */
#define LZ11_MAX_LEN 65808

/** @brief LZ11 maximum displacement */
#define LZ11_MAX_DISP 4096

namespace Tex3DS
{

namespace
{
/** @brief LZ compression mode */
enum LZSS_t
{
	LZ10, ///< LZSS/LZ10 compression
	LZ11, ///< LZ11 compression
};

const uint8_t *rfind (const uint8_t *first, const uint8_t *last, const uint8_t &val)
{
	assert (last >= first);
	while (--last >= first)
	{
		if (*last == val)
			return last;
	}

	return nullptr;
}

/** @brief Find best buffer match
 *  @param[in]  start     Input buffer
 *  @param[in]  buffer    Encoding buffer
 *  @param[in]  len       Length of encoding buffer
 *  @param[in]  max_disp  Maximum displacement
 *  @param[out] outlen    Length of match
 *  @returns Best match
 *  @retval nullptr no match
 */
const uint8_t *find_best_match (const uint8_t *start,
    const uint8_t *buffer,
    size_t len,
    size_t max_disp,
    size_t &outlen)
{
	assert (buffer > start);

	// clamp start to maximum displacement from buffer
	if (buffer - start > static_cast<ptrdiff_t> (max_disp))
		start = buffer - max_disp;

	const uint8_t *best_start = buffer;
	size_t best_len           = 0;

	const uint8_t *p = buffer;

	// find nearest matching start byte
	while ((p = rfind (start, p, *buffer)))
	{
		// confirm that this match is as good as the best match so far
		if (std::memcmp (p, buffer, best_len) != 0)
			continue;

		// find length of match
		size_t test_len = best_len;
		for (size_t i = best_len; i < len; ++i)
		{
			if (p[i] == buffer[i])
				++test_len;
			else
				break;
		}

		if (test_len >= best_len)
		{
			// this match is the best so far, so save it
			best_start = p;
			best_len   = test_len;
		}

		// if we maximized the match, stop here
		if (best_len == len)
			break;
	}

	if (best_len)
	{
		// we found a match, so return it
		outlen = best_len;
		return best_start;
	}

	// no match found
	outlen = 0;
	return nullptr;
}

/** @brief LZSS/LZ10/LZ11 compression
 *  @param[in]  buffer Source buffer
 *  @param[in]  len    Source length
 *  @param[out] outlen Output length
 *  @param[in]  mode   LZ mode
 *  @returns Compressed buffer
 */
std::vector<uint8_t> lzssCommonEncode (const uint8_t *buffer, size_t len, LZSS_t mode)
{
	// get maximum match length
	const size_t max_len = mode == LZ10 ? LZ10_MAX_LEN : LZ11_MAX_LEN;

	// get maximum displacement
	const size_t max_disp = mode == LZ10 ? LZ10_MAX_DISP : LZ11_MAX_DISP;

	assert (mode == LZ10 || mode == LZ11);

	// create output buffer
	std::vector<uint8_t> result;

	// append compression header
	if (mode == LZ10)
		compressionHeader (result, 0x10, len);
	else
		compressionHeader (result, 0x11, len);

	// reserve an encode byte in output buffer
	size_t code_pos = result.size ();
	result.push_back (0);

	// initialize shift
	size_t shift = 7;

	// encode every byte
	const uint8_t *start = buffer;
#ifndef NDEBUG
	const uint8_t *end = buffer + len;
#endif
	while (len > 0)
	{
		assert (buffer < end);
		assert (buffer + len == end);

		const uint8_t *tmp;
		size_t tmplen;

		if (buffer != start)
		{
			// find best match
			tmp = find_best_match (start, buffer, std::min (len, max_len), max_disp, tmplen);
			if (tmp != NULL)
			{
				assert (tmp >= start);
				assert (tmp < buffer);
				assert (buffer - tmp <= static_cast<ptrdiff_t> (max_disp));
				assert (tmplen <= max_len);
				assert (tmplen <= len);
				assert (std::memcmp (buffer, tmp, tmplen) == 0);
			}
		}
		else
		{
			// beginning of stream must be primed with at least one value
			tmplen = 1;
		}

		if (tmplen > 2 && tmplen < len)
		{
			// this match is long enough to be compressed; let's check if it's
			// cheaper to encode this byte as a copy and start compression at the
			// next byte
			size_t skip_len, next_len;

			// get best match starting at the next byte
			find_best_match (start, buffer + 1, std::min (len - 1, max_len), max_disp, skip_len);

			// check if the match is too small to compress
			if (skip_len < 3)
				skip_len = 1;

			// get best match for data following the current compressed chunk
			find_best_match (
			    start, buffer + tmplen, std::min (len - tmplen, max_len), max_disp, next_len);

			// check if the match is too small to compress
			if (next_len < 3)
				next_len = 1;

			// if compressing this chunk and the next chunk is less valuable than
			// skipping this byte and starting compression at the next byte, mark
			// this byte as being needed to copy
			if (tmplen + next_len <= skip_len + 1)
				tmplen = 1;
		}

		if (tmplen < 3)
		{
			// this is a copy chunk; append this byte to the output buffer
			result.push_back (*buffer);

			// only one byte is copied
			tmplen = 1;
		}
		else if (mode == LZ10)
		{
			// mark this chunk as compressed
			assert (code_pos < result.size ());
			result[code_pos] |= (1 << shift);

			// encode the displacement and length
			size_t disp = buffer - tmp - 1;
			assert (tmplen - 3 <= 0xF);
			assert (disp <= 0xFFF);
			result.push_back (((tmplen - 3) << 4) | (disp >> 8));
			result.push_back (disp);
		}
		else if (tmplen <= 0x10)
		{
			// mark this chunk as compressed
			assert (code_pos < result.size ());
			result[code_pos] |= (1 << shift);

			// encode the displacement and length
			size_t disp = buffer - tmp - 1;
			assert (tmplen > 2);
			assert (tmplen - 1 <= 0xF);
			assert (disp <= 0xFFF);
			result.push_back (((tmplen - 1) << 4) | (disp >> 8));
			result.push_back (disp);
		}
		else if (tmplen <= 0x110)
		{
			// mark this chunk as compressed
			assert (code_pos < result.size ());
			result[code_pos] |= (1 << shift);

			// encode the displacement and length
			size_t disp = buffer - tmp - 1;
			assert (tmplen >= 0x11);
			assert (tmplen - 0x11 <= 0xFF);
			assert (disp <= 0xFFF);
			result.push_back ((tmplen - 0x11) >> 4);
			result.push_back (((tmplen - 0x11) << 4) | (disp >> 8));
			result.push_back (disp);
		}
		else
		{
			// mark this chunk as compressed
			assert (code_pos < result.size ());
			result[code_pos] |= (1 << shift);

			// encode the displacement and length
			size_t disp = buffer - tmp - 1;
			assert (tmplen >= 0x111);
			assert (tmplen - 0x111 <= 0xFFFF);
			assert (disp <= 0xFFF);
			result.push_back ((1 << 4) | (tmplen - 0x111) >> 12);
			result.push_back (((tmplen - 0x111) >> 4));
			result.push_back (((tmplen - 0x111) << 4) | (disp >> 8));
			result.push_back (disp);
		}

		// advance input buffer
		buffer += tmplen;
		len -= tmplen;

		if (shift == 0 && len != 0)
		{
			// we need to encode more data, so add a new code byte
			shift    = 8;
			code_pos = result.size ();
			result.push_back (0);
		}

		// advance code byte bit position
		if (shift != 0)
			--shift;
	}

	// pad the output buffer to 4 bytes
	if (result.size () & 0x3)
		result.resize ((result.size () + 3) & ~0x3);

	// return the output data
	return result;
}
}

std::vector<uint8_t> lzssEncode (const void *src, size_t len)
{
	return lzssCommonEncode (reinterpret_cast<const uint8_t *> (src), len, LZ10);
}

std::vector<uint8_t> lz11Encode (const void *src, size_t len)
{
	return lzssCommonEncode (reinterpret_cast<const uint8_t *> (src), len, LZ11);
}

void lzssDecode (const void *source, void *dest, size_t size)
{
	const uint8_t *src = (const uint8_t *)source;
	uint8_t *dst       = (uint8_t *)dest;
	uint8_t flags      = 0;
	uint8_t mask       = 0;
	unsigned len;
	unsigned disp;

	while (size > 0)
	{
		if (mask == 0)
		{
			// read in the flags data
			// from bit 7 to bit 0:
			//     0: raw byte
			//     1: compressed block
			flags = *src++;
			mask  = 0x80;
		}

		if (flags & mask) // compressed block
		{
			// disp: displacement
			// len:  length
			len  = (((*src) & 0xF0) >> 4) + 3;
			disp = ((*src++) & 0x0F);
			disp = disp << 8 | (*src++);

			if (len > size)
				len = size;

			size -= len;

			// for len, copy data from the displacement
			// to the current buffer position
			for (uint8_t *p = dst - disp - 1; len > 0; --len)
				*dst++ = *p++;
		}
		else
		{ // uncompressed block
			// copy a raw byte from the input to the output
			*dst++ = *src++;
			size--;
		}

		mask >>= 1;
	}
}

void lz11Decode (const void *source, void *dest, size_t size)
{
	const uint8_t *src = (const uint8_t *)source;
	uint8_t *dst       = (uint8_t *)dest;
	int i;
	uint8_t flags;

	while (size > 0)
	{
		// read in the flags data
		// from bit 7 to bit 0, following blocks:
		//     0: raw byte
		//     1: compressed block
		flags = *src++;
		for (i = 0; i < 8 && size > 0; i++, flags <<= 1)
		{
			if (flags & 0x80) // compressed block
			{
				size_t len;  // length
				size_t disp; // displacement
				switch ((*src) >> 4)
				{
				case 0: // extended block
					len = (*src++) << 4;
					len |= ((*src) >> 4);
					len += 0x11;
					break;
				case 1: // extra extended block
					len = ((*src++) & 0x0F) << 12;
					len |= (*src++) << 4;
					len |= ((*src) >> 4);
					len += 0x111;
					break;
				default: // normal block
					len = ((*src) >> 4) + 1;
					break;
				}

				disp = ((*src++) & 0x0F) << 8;
				disp |= *src++;

				if (len > size)
					len = size;

				size -= len;

				// for len, copy data from the displacement
				// to the current buffer position
				for (uint8_t *p = dst - disp - 1; len > 0; --len)
					*dst++ = *p++;
			}

			else
			{ // uncompressed block
				// copy a raw byte from the input to the output
				*dst++ = *src++;
				--size;
			}
		}
	}
}

} // namespace Tex3DS
