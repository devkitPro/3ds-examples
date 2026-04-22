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
/** @file subimage.h
 *  @brief Sub-image structures
 */
#pragma once

#include "libtex3ds.h"

#include <libgen.h>

#include <cassert>
#include <cstdio>
#include <string>

namespace Tex3DS
{

struct SubImage
{
	size_t index;     ///< Sorting order
	std::string name; ///< Sub-image name
	float left;       ///< Left u-coordinate
	float top;        ///< Top v-coordinate
	float right;      ///< Right u-coordinate
	float bottom;     ///< Bottom v-coordinate
	bool rotated;     ///< Whether sub-image is rotated

	SubImage (size_t index,
	    const std::string &name,
	    float left,
	    float top,
	    float right,
	    float bottom,
	    bool rotated)
	    : index (index),
	      name (name),
	      left (left),
	      top (top),
	      right (right),
	      bottom (bottom),
	      rotated (rotated)
	{
		assert (rotated == (top < bottom));

		if (name.empty ())
			return;

		size_t pos = name.find_last_of ("/\\");
		this->name = (pos != std::string::npos) ? name.substr (pos + 1) : name;
	}

	bool operator< (const SubImage &rhs) const
	{
		return index < rhs.index;
	}

	void print (unsigned width, unsigned height) const
	{
		if (rotated)
			std::printf ("%4zu \"%s\"\n"
			             "\ttl %5.1lf %5.1lf\n"
			             "\ttr %5.1lf %5.1lf\n"
			             "\tbl %5.1lf %5.1lf\n"
			             "\tbr %5.1lf %5.1lf\n"
			             "\trotated\n",
			    index,
			    name.c_str (),
			    top * width,
			    left * height,
			    top * width,
			    right * height,
			    bottom * width,
			    left * height,
			    bottom * width,
			    right * height);

		else
			std::printf ("%4zu \"%s\"\n"
			             "\ttl %5.1lf %5.1lf\n"
			             "\ttr %5.1lf %5.1lf\n"
			             "\tbl %5.1lf %5.1lf\n"
			             "\tbr %5.1lf %5.1lf\n",
			    index,
			    name.c_str (),
			    left * width,
			    top * height,
			    right * width,
			    top * height,
			    left * width,
			    bottom * height,
			    right * width,
			    bottom * height);
	}
};

} // namespace Tex3DS
