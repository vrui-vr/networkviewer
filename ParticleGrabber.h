/***********************************************************************
ParticleGrabber - Tool class to grab and drag particles in a particle
system.
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

#ifndef PARTICLEGRABBER_INCLUDED
#define PARTICLEGRABBER_INCLUDED

#include <vector>
#include <Vrui/Tool.h>

#include "ParticleSystem.h"
#include "Body.h"

/* Forward declarations: */
class ParticleGrabberFactory;

class ParticleGrabber:public Vrui::Tool // Particle grabber tool class
	{
	friend class ParticleGrabberFactory;
	
	/* Elements: */
	private:
	static ParticleGrabberFactory* factory; // Pointer to the factory object for this class
	bool particleGrabbed; // Flag if a particle is currently grabbed
	Index particleIndex; // Index of grabbed particle
	Scalar particleInvMass; // Original inverse mass of grabbed particle
	Vrui::Point particlePos; // Position of grabbed particle in input device coordinates
	Body* grabbedBody; // Pointer to the currently grabbed body, or 0
	Body::GrabID grabId; // Grab ID on the currently grabbed body
	
	/* Constructors and destructors: */
	public:
	static void initClass(ParticleSystem& sParticles,std::vector<Body*>* sBodies =0); // Initializes the particle grabber tool class to work with the given particle system and list of bodies, if pointer is !=0
	ParticleGrabber(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	virtual ~ParticleGrabber(void);
	
	/* Methods from class Vrui::Tool: */
	virtual const Vrui::ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

#endif
