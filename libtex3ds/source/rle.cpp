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
/** @file rle.c
 *  @brief Run-length encoding compression routines
 */

#include "compress.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

/** @brief Minimum run length */
#define RLE_MIN_RUN 3

/** @brief Maximum run length */
#define RLE_MAX_RUN 130

/** @brief Maximum copy length */
#define RLE_MAX_COPY 128

namespace Tex3DS
{

std::vector<uint8_t> rleEncode (const void *source, size_t len)
{
	// create output buffer
	std::vector<uint8_t> result;

	// append compression header
	compressionHeader (result, 0x30, len);

	// encode all bytes
	const uint8_t *src  = (const uint8_t *)source;
	const uint8_t *save = src, *end = src + len;
	size_t save_len = 0, run;
	while (src < end)
	{
		// calculate current run
		for (run = 1; src + run < end && run < RLE_MAX_RUN; ++run)
		{
			if (src[run] != *src)
				break;
		}

		if (run < RLE_MIN_RUN)
		{
			// run not long enough to encode
			++src;
			++save_len;
		}

		// check if we need to encode a copy
		if (save_len == RLE_MAX_COPY || (save_len > 0 && run > 2))
		{
			// append encoded copy length followed by copy buffer
			assert (save_len - 1 < RLE_MAX_COPY);
			result.push_back (save_len - 1);
			result.insert (std::end (result), save, save + save_len);

			// reset save point
			save += save_len;
			save_len = 0;
		}

		// check if run is long enough to encode
		if (run > 2)
		{
			// append encoded run to output buffer
			assert (run - 3 < RLE_MAX_RUN);
			result.push_back (0x80 | (run - 3));
			result.push_back (*src);

			// reset save point
			src += run;
			save = src;
			assert (save_len == 0);
		}
	}

	assert (save + save_len == end);

	// check if there is data left to copy
	if (save_len)
	{
		// append encoded copy length followed by copy buffer
		assert (save_len - 1 < RLE_MAX_COPY);
		result.push_back (save_len - 1);
		result.insert (std::end (result), save, save + save_len);
	}

	// pad the output buffer to 4 bytes
	if (result.size () & 0x3)
		result.resize ((result.size () + 3) & ~0x3);

	// return the output data
	return result;
}

void rleDecode (const void *source, void *dest, size_t size)
{
	const uint8_t *src = reinterpret_cast<const uint8_t *> (source);
	uint8_t *dst       = reinterpret_cast<uint8_t *> (dest);
	uint8_t byte;
	size_t len;

	while (size > 0)
	{
		// read in the data header
		byte = *src++;

		if (byte & 0x80) // compressed block
		{
			// read the length of the run
			len = (byte & 0x7F) + 3;

			if (len > size)
				len = size;

			size -= len;

			// read in the byte used for the run
			byte = *src++;

			// for len, copy byte into output
			memset (dst, byte, len);
			dst += len;
		}
		else // uncompressed block
		{
			// read the length of uncompressed bytes
			len = (byte & 0x7F) + 1;

			if (len > size)
				len = size;

			size -= len;

			// for len, copy from input to output
			memcpy (dst, src, len);
			dst += len;
			src += len;
		}
	}
}

} // namespace Tex3DS
