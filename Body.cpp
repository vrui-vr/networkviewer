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

#include "Body.h"

/*********************
Methods of class Body:
*********************/

Body::~Body(void)
	{
	}

Body::GrabID Body::grab(const Point& grabPos,Scalar grabRadius,const Body::GrabTransform& initialGrabTransform)
	{
	/* Default grab is unsuccessful: */
	return GrabID(0);
	}

void Body::grabUpdate(Body::GrabID grabId,const Body::GrabTransform& newGrabTransform)
	{
	}

void Body::grabRelease(Body::GrabID grabId)
	{
	}

void Body::applyForces(Scalar dt,Scalar dt2)
	{
	}

void Body::update(Scalar dt)
	{
	}

void Body::glRenderAction(GLContextData& contextData) const
	{
	}

