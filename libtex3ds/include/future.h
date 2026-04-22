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
/** @file future.h
 *  @brief C++ future compatibility.
 */
#pragma once

#include <memory>

namespace Tex3DS
{

namespace future
{
#if __cplusplus < 201402L
template <typename T, typename... Args>
inline std::unique_ptr<T> make_unique (Args &&... args)
{
	return std::unique_ptr<T> (new T (std::forward<Args> (args)...));
}
#else
using std::make_unique;
#endif
}

}
