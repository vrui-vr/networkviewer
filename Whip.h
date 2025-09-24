/***********************************************************************
Whip - Class representing an Indiana Jones(tm) bull whip made out of
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

#ifndef WHIP_INCLUDED
#define WHIP_INCLUDED

#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>

#include "Body.h"

class Whip:public Body
	{
	/* Embedded classes: */
	private:
	struct Grab // Structure identifying a grab
		{
		/* Elements: */
		public:
		unsigned int grabbedSegment; // Index of currently grabbed whip segment
		Point grabbedParticlePos[2]; // Positions of the two particles defining the grabbed whip segment in grabber coordinates
		};
	
	typedef Misc::HashTable<GrabID,Grab> GrabMap; // Hash table mapping grab IDs to grab structures
	
	/* Elements: */
	private:
	unsigned int numParticles; // Number of particles in the whip and whip handle
	Index* particleIndices; // Indices of the whip particles in the particle system
	GrabMap grabs; // Map of currently active grabs
	GrabID nextGrabId; // ID to be assigned to next successful grab attempt
	
	/* Constructors and destructors: */
	public:
	Whip(ParticleSystem& sParticles,const Point& position,const Vector& direction);
	virtual ~Whip(void);
	
	/* Methods from class Body: */
	virtual GrabID grab(const Point& grabPos,Scalar grabRadius,const GrabTransform& initialGrabTransform);
	virtual void grabUpdate(GrabID grabId,const GrabTransform& newGrabTransform);
	virtual void grabRelease(GrabID grabId);
	virtual void applyForces(Scalar dt,Scalar dt2);
	virtual void update(Scalar dt);
	virtual void glRenderAction(GLContextData& contextData) const;
	};

#endif
