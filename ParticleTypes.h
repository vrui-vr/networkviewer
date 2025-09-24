/***********************************************************************
ParticleTypes - Declarations of basic types to represent and work with
particles.
Copyright (c) 2018-2020 Oliver Kreylos

This file is part of the Network Viewer.

The Network Viewer is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Network Viewer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Network Viewer; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef PARTICLETYPES_INCLUDED
#define PARTICLETYPES_INCLUDED

#include <Misc/SizedTypes.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

typedef Misc::UInt32 Index; // Type for particle indices
typedef Misc::Float64 Scalar; // Scalar type for points and vectors
typedef Geometry::Point<Scalar,3> Point; // Type for points
typedef Geometry::Vector<Scalar,3> Vector; // Type for vectors

#endif
