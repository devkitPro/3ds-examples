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
/** @file swizzle.h
 *  @brief Swizzle routines
 */
#pragma once

#include "libtex3ds.h"

namespace Tex3DS
{

/** @brief Swizzle an image (Morton order)
 *  @param[in] img     Image to swizzle
 *  @param[in] reverse Whether to unswizzle
 */
void swizzle (Tex3DS::Image &img, bool reverse);

} // namespace Tex3DS
