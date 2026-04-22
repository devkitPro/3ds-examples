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
/** @file quantum.h
 *  @brief Tex3DS::Quantum conversions
 */
#pragma once

#include "libtex3ds.h"

#include <algorithm>
#include <cmath>

namespace Tex3DS
{

namespace
{

inline Tex3DS::Quantum quantumRed(const Tex3DS::RGBA& c)
{
	return c.r;
}

inline Tex3DS::Quantum quantumGreen(const Tex3DS::RGBA& c)
{
	return c.g;
}

inline Tex3DS::Quantum quantumBlue(const Tex3DS::RGBA& c)
{
	return c.b;
}

inline Tex3DS::Quantum quantumAlpha(const Tex3DS::RGBA& c)
{
	return c.a;
}

inline void quantumRed(Tex3DS::RGBA& c, Tex3DS::Quantum q)
{
	c.r = q;
}

inline void quantumGreen(Tex3DS::RGBA& c, Tex3DS::Quantum q)
{
	c.g = q;
}

inline void quantumBlue(Tex3DS::RGBA& c, Tex3DS::Quantum q)
{
	c.b = q;
}

inline void quantumAlpha(Tex3DS::RGBA& c, Tex3DS::Quantum q)
{
	c.a = q;
}

/** @brief Convert Tex3DS::Quantum to an n-bit value
 *  @tparam    bits Number of bits for output value
 *  @param[in] v    Quantum to convert
 *  @returns n-bit quantum
 */
template <int bits>
inline uint8_t quantum_to_bits (Tex3DS::Quantum v)
{
	return (1 << bits) * v / (Tex3DS::QuantumRange + 1);
}

/** @brief Convert an n-bit value to a Tex3DS::Quantum
 *  @tparam    bits Number of bits for input value
 *  @param[in] v    Input n-bit value
 *  @returns Tex3DS::Quantum
 */
template <int bits>
inline Tex3DS::Quantum bits_to_quantum (uint8_t v)
{
	return v * Tex3DS::QuantumRange / ((1 << bits) - 1);
}

/** @brief Quantize a Tex3DS::Quantum to its n-bit equivalent
 *  @tparam    bits Number of significant bits
 *  @param[in] v    Quantum to quantize
 *  @returns quantized Tex3DS::Quantum
 */
template <int bits>
inline Tex3DS::Quantum quantize (Tex3DS::Quantum v)
{
	return bits_to_quantum<bits> (quantum_to_bits<bits> (v));
}

/** @brief sRGB Gamma inverse
 *  @param[in] v Value to get inverse gamma
 *  @return inverse gamma
 */
inline double gamma_inverse (double v)
{
	if (v <= 0.04045)
		return v / 12.92;
	return std::pow ((v + 0.055) / 1.055, 2.4);
}

/** @brief sRGB Gamma
 *  @param[in] v Value to get gamma
 *  @return gamma
 */
inline double gamma (double v)
{
	if (v <= 0.0031308)
		return v * 12.92;
	return 1.055 * std::pow (v, 1.0 / 2.4) - 0.055;
}

/** @brief Get luminance from RGB with gamma correction
 *  @param[in] c Color to get luminance
 *  @return luminance
 */
inline Tex3DS::Quantum luminance (const Tex3DS::RGBA &c)
{
	// ITU Recommendation BT.709
	const double r = 0.212655;
	const double g = 0.715158;
	const double b = 0.072187;

	// Gamma correction
	double v = gamma (r * gamma_inverse (static_cast<double> (quantumRed (c)) / Tex3DS::QuantumRange) +
	                  g * gamma_inverse (static_cast<double> (quantumGreen (c)) / Tex3DS::QuantumRange) +
	                  b * gamma_inverse (static_cast<double> (quantumBlue (c)) / Tex3DS::QuantumRange));

	// clamp
	return std::max (0.0, std::min (1.0, v)) * Tex3DS::QuantumRange;
}

}

} // namespace Tex3DS
