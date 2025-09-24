/***********************************************************************
Figure - Class representing an articulated figure read from a figure
definition file.
Copyright (c) 2019-2020 Oliver Kreylos

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

#ifndef FIGURE_INCLUDED
#define FIGURE_INCLUDED

#include <vector>
#include <Misc/UnorderedTuple.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>

#include "ParticleMesh.h"
#include "Body.h"

class Figure:public Body
	{
	/* Embedded classes: */
	private:
	typedef Misc::UnorderedTuple<Index,2> Handle; // Class defining a grabbable handle as a line between two vertices
	struct Grab // Structure identifying a grab
		{
		/* Elements: */
		public:
		unsigned int grabbedHandleIndex; // Index of currently grabbed figure handle
		Point grabbedParticlePos[2]; // Positions of the two particles defining the grabbed figure handle in grabber coordinates
		Scalar grabbedParticleInvMass[2]; // Original inverse masses of the grabbed particles
		};
	
	typedef Misc::HashTable<GrabID,Grab> GrabMap; // Hash table mapping grab IDs to grab structures
	
	/* Elements: */
	std::vector<Index> vertexIndices; // Indices of the figure vertices in the particle system
	ParticleMesh figureMesh; // Mesh defining the figure's appearance
	Scalar handleRadius; // Radius of figure handles
	std::vector<Handle> handles; // List of handles to grab the figure
	GrabMap grabs; // Map of currently active grabs
	GrabID nextGrabId; // ID to be assigned to next successful grab attempt
	
	/* Constructors and destructors: */
	public:
	Figure(ParticleSystem& sParticles,const char* figureFileName,const GrabTransform& initialTransform); // Creates a figure from the given figure definition file at the given position and orientation
	virtual ~Figure(void);
	
	/* Methods from class Body: */
	virtual GrabID grab(const Point& grabPos,Scalar grabRadius,const GrabTransform& initialGrabTransform);
	virtual void grabUpdate(GrabID grabId,const GrabTransform& newGrabTransform);
	virtual void grabRelease(GrabID grabId);
	virtual void update(Scalar dt);
	virtual void glRenderAction(GLContextData& contextData) const;
	};

#endif
