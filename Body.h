/***********************************************************************
Body - Base class for rigid, soft, or articulated bodies made out of
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

#ifndef BODY_INCLUDED
#define BODY_INCLUDED

#include "ParticleTypes.h"

/* Forward declarations: */
namespace Geometry {
template <class ScalarParam,int dimensionParam>
class OrthonormalTransformation;
}
class GLContextData;
class ParticleSystem;

class Body
	{
	/* Embedded classes: */
	public:
	typedef unsigned int GrabID; // ID for a grab
	typedef Geometry::OrthonormalTransformation<Scalar,3> GrabTransform; // Type for grabbing transformations
	
	/* Elements: */
	protected:
	ParticleSystem& particles; // Reference to the particle system containing the particles defining this body
	
	/* Constructors and destructors: */
	public:
	Body(ParticleSystem& sParticles)
		:particles(sParticles)
		{
		}
	virtual ~Body(void);
	
	/* Methods: */
	virtual GrabID grab(const Point& grabPos,Scalar grabRadius,const GrabTransform& initialGrabTransform); // Grabs a body at the given position; returns valid grab ID if body was successfully grabbed, or zero if not
	virtual void grabUpdate(GrabID grabId,const GrabTransform& newGrabTransform); // Updates the state of a grabbed body before the particle system is updated
	virtual void grabRelease(GrabID grabId); // Releases a grabbed body
	virtual void applyForces(Scalar dt,Scalar dt2); // Lets the body apply forces during an update of the particle system
	virtual void update(Scalar dt); // Updates the body after the particle system's state has been advanced by the given time step
	virtual void glRenderAction(GLContextData& contextData) const; // Renders the body in its current state
	};

#endif
