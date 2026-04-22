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
/** @file encode.cpp
 *  @brief Image encoding routines
 */

#include "encode.h"
#include "quantum.h"
#include "rg_etc1.h"

namespace Tex3DS
{

namespace
{
/** @brief ETC1/ETC1A4 encoder
 *  @param[in] work  Work unit
 *  @param[in] alpha Whether to output alpha data
 */
void etc1_common (encode::WorkUnit &work, bool alpha)
{
	rg_etc1::etc1_pack_params params;
	params.clear ();
	params.m_quality = work.etc1_quality;

	for (size_t j = 0; j < 8; j += 4)
	{
		for (size_t i = 0; i < 8; i += 4)
		{
			uint8_t in_block[4 * 4 * 4];
			uint8_t out_block[8];
			uint8_t out_alpha[8] = {0, 0, 0, 0, 0, 0, 0, 0};

			if (work.output || work.preview)
			{
				// iterate each 4x4 subblock
				for (size_t y = 0; y < 4; ++y)
				{
					for (size_t x = 0; x < 4; ++x)
					{
						Tex3DS::RGBA c = work.p[(j + y) * work.stride + i + x];

						in_block[y * 16 + x * 4 + 0] = quantum_to_bits<8> (quantumRed (c));
						in_block[y * 16 + x * 4 + 1] = quantum_to_bits<8> (quantumGreen (c));
						in_block[y * 16 + x * 4 + 2] = quantum_to_bits<8> (quantumBlue (c));
						in_block[y * 16 + x * 4 + 3] = 0xFF;

						if (alpha && work.output)
						{
							// encode 4bpp alpha; X/Y axes are swapped
							if (y & 1)
								out_alpha[2 * x + y / 2] |=
								    (quantum_to_bits<4> (quantumAlpha (c)) << 4);
							else
								out_alpha[2 * x + y / 2] |=
								    (quantum_to_bits<4> (quantumAlpha (c)) << 0);
						}
					}
				}

				// encode etc1 block
				rg_etc1::pack_etc1_block (
				    out_block, reinterpret_cast<unsigned *> (in_block), params);
			}

			if (work.output)
			{
				// alpha block precedes etc1 block
				if (alpha)
				{
					for (size_t i = 0; i < 8; ++i)
						work.result.push_back (out_alpha[i]);
				}

				// rg_etc1 outputs in big-endian; convert to little-endian
				for (size_t i = 0; i < 8; ++i)
					work.result.push_back (out_block[8 - i - 1]);
			}

			if (work.preview)
			{
				rg_etc1::unpack_etc1_block (out_block, reinterpret_cast<unsigned *> (in_block));

				for (size_t y = 0; y < 4; ++y)
				{
					for (size_t x = 0; x < 4; ++x)
					{
						Tex3DS::RGBA c = work.p[(j + y) * work.stride + i + x];

						quantumRed (c, bits_to_quantum<8> (in_block[y * 16 + x * 4 + 0]));
						quantumGreen (c, bits_to_quantum<8> (in_block[y * 16 + x * 4 + 1]));
						quantumBlue (c, bits_to_quantum<8> (in_block[y * 16 + x * 4 + 2]));

						if (alpha)
							quantumAlpha (c, quantize<4> (quantumAlpha (c)));
						else
						{
							quantumAlpha (c, Tex3DS::QuantumRange);
						}

						work.p[(j + y) * work.stride + i + x] = c;
					}
				}
			}
		}
	}
}
}

namespace encode
{
void rgba8888 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
			{
				work.result.push_back (quantum_to_bits<8> (quantumAlpha (c)));
				work.result.push_back (quantum_to_bits<8> (quantumBlue (c)));
				work.result.push_back (quantum_to_bits<8> (quantumGreen (c)));
				work.result.push_back (quantum_to_bits<8> (quantumRed (c)));
			}

			if (work.preview)
			{
				quantumRed (c, quantize<8> (quantumRed (c)));
				quantumGreen (c, quantize<8> (quantumGreen (c)));
				quantumBlue (c, quantize<8> (quantumBlue (c)));
				quantumAlpha (c, quantize<8> (quantumAlpha (c)));

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void rgb888 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
			{
				work.result.push_back (quantum_to_bits<8> (quantumBlue (c)));
				work.result.push_back (quantum_to_bits<8> (quantumGreen (c)));
				work.result.push_back (quantum_to_bits<8> (quantumRed (c)));
			}

			if (work.preview)
			{
				quantumRed (c, quantize<8> (quantumRed (c)));
				quantumGreen (c, quantize<8> (quantumGreen (c)));
				quantumBlue (c, quantize<8> (quantumBlue (c)));
				quantumAlpha (c, Tex3DS::QuantumRange);

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void rgba5551 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
			{
				uint16_t v = (quantum_to_bits<5> (quantumRed (c)) << 11) |
				             (quantum_to_bits<5> (quantumGreen (c)) << 6) |
				             (quantum_to_bits<5> (quantumBlue (c)) << 1) |
				             (quantum_to_bits<1> (quantumAlpha (c)) << 0);

				work.result.push_back (v >> 0);
				work.result.push_back (v >> 8);
			}

			if (work.preview)
			{
				quantumRed (c, quantize<5> (quantumRed (c)));
				quantumGreen (c, quantize<5> (quantumGreen (c)));
				quantumBlue (c, quantize<5> (quantumBlue (c)));
				quantumAlpha (c, quantize<1> (quantumAlpha (c)));

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void rgb565 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
			{
				uint16_t v = (quantum_to_bits<5> (quantumRed (c)) << 11) |
				             (quantum_to_bits<6> (quantumGreen (c)) << 5) |
				             (quantum_to_bits<5> (quantumBlue (c)) << 0);

				work.result.push_back (v >> 0);
				work.result.push_back (v >> 8);
			}

			if (work.preview)
			{
				quantumRed (c, quantize<5> (quantumRed (c)));
				quantumGreen (c, quantize<6> (quantumGreen (c)));
				quantumBlue (c, quantize<5> (quantumBlue (c)));
				quantumAlpha (c, Tex3DS::QuantumRange);

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void rgba4444 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
			{
				uint16_t v = (quantum_to_bits<4> (quantumRed (c)) << 12) |
				             (quantum_to_bits<4> (quantumGreen (c)) << 8) |
				             (quantum_to_bits<4> (quantumBlue (c)) << 4) |
				             (quantum_to_bits<4> (quantumAlpha (c)) << 0);

				work.result.push_back (v >> 0);
				work.result.push_back (v >> 8);
			}

			if (work.preview)
			{
				quantumRed (c, quantize<4> (quantumRed (c)));
				quantumGreen (c, quantize<4> (quantumGreen (c)));
				quantumBlue (c, quantize<4> (quantumBlue (c)));
				quantumAlpha (c, quantize<4> (quantumAlpha (c)));

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void la88 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
			{
				work.result.push_back (quantum_to_bits<8> (quantumAlpha (c)));
				work.result.push_back (quantum_to_bits<8> (luminance (c)));
			}

			if (work.preview)
			{
				Tex3DS::Quantum l = quantize<8> (luminance (c));

				quantumRed (c, l);
				quantumGreen (c, l);
				quantumBlue (c, l);
				quantumAlpha (c, quantize<8> (quantumAlpha (c)));

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void hilo88 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
			{
				work.result.push_back (quantum_to_bits<8> (quantumGreen (c)));
				work.result.push_back (quantum_to_bits<8> (quantumRed (c)));
			}

			if (work.preview)
			{
				quantumRed (c, quantize<8> (quantumRed (c)));
				quantumGreen (c, quantize<8> (quantumGreen (c)));
				quantumBlue (c, 0);
				quantumAlpha (c, Tex3DS::QuantumRange);

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void l8 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
				work.result.push_back (quantum_to_bits<8> (luminance (c)));

			if (work.preview)
			{
				Tex3DS::Quantum l = quantize<8> (luminance (c));

				quantumRed (c, l);
				quantumGreen (c, l);
				quantumBlue (c, l);
				quantumAlpha (c, Tex3DS::QuantumRange);

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void a8 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
				work.result.push_back (quantum_to_bits<8> (quantumAlpha (c)));

			if (work.preview)
			{
				quantumRed (c, 0);
				quantumGreen (c, 0);
				quantumBlue (c, 0);
				quantumAlpha (c, quantize<8> (quantumAlpha (c)));

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void la44 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			Tex3DS::RGBA c = work.p[j * work.stride + i];

			if (work.output)
			{
				work.result.push_back ((quantum_to_bits<4> (luminance (c)) << 4) |
				                       (quantum_to_bits<4> (quantumAlpha (c)) << 0));
			}

			if (work.preview)
			{
				Tex3DS::Quantum l = quantize<4> (luminance (c));

				quantumRed (c, l);
				quantumGreen (c, l);
				quantumBlue (c, l);
				quantumAlpha (c, quantize<4> (quantumAlpha (c)));

				work.p[j * work.stride + i] = c;
			}
		}
	}
}

void l4 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; i += 2)
		{
			Tex3DS::RGBA c1 = work.p[j * work.stride + i + 0],
			              c2 = work.p[j * work.stride + i + 1];

			if (work.output)
			{
				work.result.push_back ((quantum_to_bits<4> (luminance (c2)) << 4) |
				                       (quantum_to_bits<4> (luminance (c1)) << 0));
			}

			if (work.preview)
			{
				Tex3DS::Quantum l = quantize<4> (luminance (c1));

				quantumRed (c1, l);
				quantumGreen (c1, l);
				quantumBlue (c1, l);
				quantumAlpha (c1, Tex3DS::QuantumRange);

				l = quantize<4> (luminance (c2));

				quantumRed (c2, l);
				quantumGreen (c2, l);
				quantumBlue (c2, l);
				quantumAlpha (c2, Tex3DS::QuantumRange);

				work.p[j * work.stride + i + 0] = c1;
				work.p[j * work.stride + i + 1] = c2;
			}
		}
	}
}

void a4 (WorkUnit &work)
{
	for (size_t j = 0; j < 8; ++j)
	{
		for (size_t i = 0; i < 8; i += 2)
		{
			Tex3DS::RGBA c1 = work.p[j * work.stride + i + 0],
			              c2 = work.p[j * work.stride + i + 1];

			if (work.output)
			{
				work.result.push_back ((quantum_to_bits<4> (quantumAlpha (c2)) << 4) |
				                       (quantum_to_bits<4> (quantumAlpha (c1)) << 0));
			}

			if (work.preview)
			{
				quantumRed (c1, 0);
				quantumGreen (c1, 0);
				quantumBlue (c1, 0);
				quantumAlpha (c1, quantize<4> (quantumAlpha (c1)));

				quantumRed (c2, 0);
				quantumGreen (c2, 0);
				quantumBlue (c2, 0);
				quantumAlpha (c2, quantize<4> (quantumAlpha (c2)));

				work.p[j * work.stride + i + 0] = c1;
				work.p[j * work.stride + i + 1] = c2;
			}
		}
	}
}

void etc1 (WorkUnit &work)
{
	etc1_common (work, false);
}

void etc1a4 (WorkUnit &work)
{
	etc1_common (work, true);
}
}

} // namespace Tex3DS
