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
/** @file compress.h
 *  @brief Compression routines
 */
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Tex3DS
{

/** @brief LZSS/LZ10 compression
 *  @param[in] src Source buffer
 *  @param[in] len Source length
 *  @returns Compressed buffer
 */
std::vector<uint8_t> lzssEncode (const void *src, size_t len);

/** @brief LZSS/LZ10 decompression
 *  @param[in]  src Source buffer
 *  @param[out] dst Destination buffer
 *  @param[in]  len Destination length
 *  @note The output buffer must be large enough to hold the decompressed data
 */
void lzssDecode (const void *src, void *dst, size_t len);

/** @brief LZ11 compression
 *  @param[in] src Source buffer
 *  @param[in] len Source length
 *  @returns Compressed buffer
 */
std::vector<uint8_t> lz11Encode (const void *src, size_t len);

/** @brief LZ11 decompression
 *  @param[in]  src Source buffer
 *  @param[out] dst Destination buffer
 *  @param[in]  len Destination length
 *  @note The output buffer must be large enough to hold the decompressed data
 */
void lz11Decode (const void *src, void *dst, size_t len);

/** @brief Run-length encoding compression
 *  @param[in] src Source buffer
 *  @param[in] len Source length
 *  @returns Compressed buffer
 */
std::vector<uint8_t> rleEncode (const void *src, size_t len);

/** @brief Run-length encoding decompression
 *  @param[in]  src Source buffer
 *  @param[out] dst Destination buffer
 *  @param[in]  len Destination length
 *  @note The output buffer must be large enough to hold the decompressed data
 */
void rleDecode (const void *src, void *dst, size_t len);

/** @brief Huffman compression
 *  @param[in] src Source buffer
 *  @param[in] len Source length
 *  @returns Compressed buffer
 */
std::vector<uint8_t> huffEncode (const void *src, size_t len);

/** @brief Huffman decompression
 *  @param[in]  src Source buffer
 *  @param[out] dst Destination buffer
 *  @param[in]  len Destination length
 *  @note The output buffer must be large enough to hold the decompressed data
 */
void huffDecode (const void *src, void *dst, size_t len);

namespace
{
/** @brief Output a GBA-style compression header
 *  @param[out] header Output header
 *  @param[in]  type   Compression type
 *  @param[in]  size   Uncompressed data size
 *  @returns Size of the compression header
 */
inline void compressionHeader (std::vector<uint8_t> &buffer, uint8_t type, size_t size)
{
	assert (!(type & 0x80));

	buffer.push_back (type);
	buffer.push_back (size >> 0);
	buffer.push_back (size >> 8);
	buffer.push_back (size >> 16);

	if (size >= 0x1000000)
	{
		buffer[buffer.size () - 4] |= 0x80;
		buffer.push_back (size >> 24);
		buffer.push_back (0); /* Reserved */
		buffer.push_back (0); /* Reserved */
		buffer.push_back (0); /* Reserved */
	}
}
}

} // namespace Tex3DS
