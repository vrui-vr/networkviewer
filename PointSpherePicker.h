/***********************************************************************
PointSpherePicker - Functor class to pick spheres based on distance to a
query point.
Copyright (c) 2010-2020 Oliver Kreylos

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

#ifndef POINTSPHEREPICKER_INCLUDED
#define POINTSPHEREPICKER_INCLUDED

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>

#include "ParticleTypes.h"

class PointSpherePicker
	{
	/* Elements: */
	private:
	Point queryPoint; // The query point
	Scalar maxPickDist; // Maximum distance from query point to surface of a sphere to register a pick
	Index sphereIndex; // Index of the next sphere to be picked
	Index pickIndex; // Index of currently picked sphere, or ~0x0
	Scalar pickDist2; // Squared distance from query point to center of currently picked sphere
	
	/* Constructors and destructors: */
	public:
	PointSpherePicker(const Point& sQueryPoint,Scalar sMaxPickDist) // Creates a point sphere picker for the given query point and maximum picking distance
		:queryPoint(sQueryPoint),
		 maxPickDist(sMaxPickDist),
		 sphereIndex(0),
		 pickIndex(~Index(0)),pickDist2(Math::Constants<Scalar>::max)
		{
		}
	
	/* Methods: */
	bool operator()(const Point& center,Scalar radius) // Checks if the given sphere registers as a pick and is closer than the previously picked sphere; returns true if sphere was picked
		{
		bool result=false;
		
		/* Reject the sphere if the distance from its center to the query point is larger than that of the currently picked sphere: */
		Scalar dist2=Geometry::sqrDist(queryPoint,center);
		if(dist2<pickDist2)
			{
			/* Check if the sphere registers as a pick: */
			if(dist2<=Math::sqr(radius+maxPickDist))
				{
				/* Pick the sphere: */
				pickIndex=sphereIndex;
				pickDist2=dist2;
				result=true;
				}
			}
		
		/* Prepare to process the next sphere: */
		++sphereIndex;
		
		return result;
		}
	Index getNumSpheres(void) const // Returns the total number of processed spheres
		{
		return sphereIndex;
		}
	bool havePickedSphere(void) const // Returns true if a sphere was picked
		{
		return pickIndex!=~Index(0);
		}
	Index getPickIndex(void) const // Returns the index of the picked sphere, or ~0x0U if no sphere was picked
		{
		return pickIndex;
		}
	Scalar getPickDist2(void) const // Returns the squared distance from the query point to the center of the picked sphere
		{
		return pickDist2;
		}
	};

#endif
