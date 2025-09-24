/***********************************************************************
RaySpherePicker - Functor class to pick spheres based on angle with a
query ray, and distance along the query ray.
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

#ifndef RAYSPHEREPICKER_INCLUDED
#define RAYSPHEREPICKER_INCLUDED

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Ray.h>

#include "ParticleTypes.h"

class RaySpherePicker
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Ray<Scalar,3> Ray; // Type for compatible rays
	
	/* Elements: */
	private:
	Ray queryRay; // The normalized query ray
	Scalar cosMaxPickAngle2; // Squared cosine of the maximum angle between query ray and surface of a sphere to register a pick
	Scalar sinMaxPickAngle; // Sine of maximum pick angle
	Index sphereIndex; // Index of the next sphere to be picked
	Index pickIndex; // Index of currently picked sphere, or ~0x0
	Scalar pickDist2; // Squared distance from query ray's starting point to center of currently picked sphere
	
	/* Constructors and destructors: */
	public:
	RaySpherePicker(const Ray& sQueryRay,Scalar sCosMaxPickAngle) // Creates a ray sphere picker for the given query ray and cosine of maximum picking angle
		:queryRay(sQueryRay),
		 cosMaxPickAngle2(Math::sqr(sCosMaxPickAngle)),sinMaxPickAngle(Math::sqrt(Scalar(1)-cosMaxPickAngle2)),
		 sphereIndex(0),
		 pickIndex(~Index(0)),pickDist2(Math::Constants<Scalar>::max)
		{
		/* Normalize the query ray's direction vector: */
		queryRay.normalizeDirection();
		}
	
	/* Methods: */
	bool operator()(const Point& center,Scalar radius) // Checks if the given sphere registers as a pick and is closer than the previously picked sphere; returns true if sphere was picked
		{
		bool result=false;
		
		/* Reject the sphere if its center is behind the query ray's start: */
		Vector d=center-queryRay.getOrigin();
		Scalar qrdd=queryRay.getDirection()*d;
		if(qrdd>=Scalar(0))
			{
			/* Reject the sphere if the query ray's start is inside the sphere, or if the distance from its center to the query ray's start is larger than that of the currently picked sphere: */
			Scalar d2=d.sqr();
			Scalar r2d2=d2-Math::sqr(radius);
			if(r2d2>Scalar(0)&&d2<pickDist2)
				{
				/***************************************************************
				Check if the sphere registers as a pick. The criterion is that
				the query ray has to intersect the sphere plus a small fuzz
				area, defined by a maximum angle deviation; i.e., the angle
				between  the query ray's direction and the vector from the query
				ray's origin to the sphere's center has to be smaller than the
				sum of the apparent angle of the sphere, seen from the query
				ray's origin, plus the fuzz angle maxPickAngle.
				***************************************************************/
				
				if(Math::sqr(qrdd+radius*sinMaxPickAngle)>=r2d2*cosMaxPickAngle2)
					{
					/* Pick the sphere: */
					pickIndex=sphereIndex;
					pickDist2=d2;
					result=true;
					}
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
