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

#include "Whip.h"

#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>

#include "ParticleSystem.h"

/*********************
Methods of class Whip:
*********************/

Whip::Whip(ParticleSystem& sParticles,const Point& position,const Vector& direction)
	:Body(sParticles),
	 numParticles(102),particleIndices(new Index[numParticles]),
	 grabs(17),nextGrabId(0)
	{
	Scalar handleLength(0.15);
	Scalar handleInvMass=Scalar(1)/Scalar(0.1);
	Scalar segmentLength(0.025);
	Scalar segmentInvMass=Scalar(1)/Scalar(0.01);
	
	Vector nd=Geometry::normalize(direction);
	
	/* Create the handle particles: */
	particleIndices[0]=particles.addParticle(handleInvMass,position,Vector::zero);
	particleIndices[1]=particles.addParticle(handleInvMass,position+nd*handleLength,Vector::zero);
	particles.addDistConstraint(particleIndices[0],particleIndices[1],handleLength);
	
	/* Create the whip particles: */
	Scalar y=handleLength+segmentLength;
	for(unsigned int i=2;i<numParticles;++i,y+=segmentLength)
		{
		particleIndices[i]=particles.addParticle(segmentInvMass,position+nd*y,Vector::zero);
		particles.addDistConstraint(particleIndices[i-1],particleIndices[i],segmentLength);
		}
	
	/* Increase the front end particle's mass: */
	particles.setParticleInvMass(particleIndices[numParticles-1],Scalar(1)/Scalar(0.05));
	
	/* Add stiffening constraints to the whip's interior particles: */
	for(unsigned int i=2;i<numParticles;++i)
		particles.addDistConstraint(particleIndices[i-2],particleIndices[i],Geometry::dist(particles.getParticlePosition(particleIndices[i-2]),particles.getParticlePosition(particleIndices[i])),Scalar(0.1));
	}

Whip::~Whip(void)
	{
	/* Remove the whip's particles from the particle system: */
	// ... we don't actually do that right now
	
	delete[] particleIndices;
	}

Body::GrabID Whip::grab(const Point& grabPos,Scalar grabRadius,const Body::GrabTransform& initialGrabTransform)
	{
	/* Check the grab position against each whip segment: */
	Point p0=particles.getParticlePosition(particleIndices[0]);
	Scalar segmentRadius=Scalar(0.02);
	unsigned int grabbedSegment=numParticles-1;
	for(unsigned int segment=1;segment<numParticles;++segment)
		{
		Point p1=particles.getParticlePosition(particleIndices[segment]);
		
		Vector d=p1-p0;
		Scalar dLen2=d.sqr();
		Vector pgp=grabPos-p0;
		Scalar dd=pgp*d;
		pgp-=d*(dd/dLen2); 
		if(pgp.sqr()<=Math::sqr(segmentRadius+grabRadius))
			{
			Scalar dLen=Math::sqrt(dLen2);
			dd/=dLen;
			if(dd>=-(segmentRadius+grabRadius)&&dd<=dLen+segmentRadius+grabRadius)
				{
				/* Found the grabbed segment: */
				grabbedSegment=segment-1;
				break;
				}
			}
		
		/* Go to the next segment: */
		p0=p1;
		segmentRadius=Scalar(0.01);
		}
	
	/* Bail out if no segment was grabbed: */
	if(grabbedSegment>=numParticles-1)
		return GrabID(0);
	
	/* Check if the grabbed segment was already grabbed: */
	for(GrabMap::Iterator gmIt=grabs.begin();!gmIt.isFinished();++gmIt)
		if(gmIt->getDest().grabbedSegment==grabbedSegment)
			{
			/* Release the segment from the existing grab and stop looking: */
			gmIt->getDest().grabbedSegment=numParticles-1;
			break;
			}
	
	/* Create a new grab: */
	Grab newGrab;
	newGrab.grabbedSegment=grabbedSegment;
	for(int i=0;i<2;++i)
		{
		Index particleIndex=particleIndices[grabbedSegment+i];
		newGrab.grabbedParticlePos[i]=initialGrabTransform.inverseTransform(particles.getParticlePosition(particleIndex));
		particles.setParticleInvMass(particleIndex,Scalar(0));
		}
	
	/* Add the grab to the map under a currently unused ID: */
	do
		{
		++nextGrabId;
		}
	while(nextGrabId==0||grabs.isEntry(nextGrabId));
	grabs[nextGrabId]=newGrab;
	
	return nextGrabId;
	}

void Whip::grabUpdate(Body::GrabID grabId,const GrabTransform& newGrabTransform)
	{
	/* Bail out if the grab does not exist or is invalid: */
	GrabMap::Iterator gIt=grabs.findEntry(grabId);
	if(gIt.isFinished()||gIt->getDest().grabbedSegment>=numParticles-1)
		return;
	
	/* Update the positions of the particles at the two ends of the grabbed segment: */
	Grab& grab=gIt->getDest();
	for(unsigned int i=0;i<2;++i)
		{
		Index particleIndex=particleIndices[grab.grabbedSegment+i];
		particles.setParticlePosition(particleIndex,newGrabTransform.transform(grab.grabbedParticlePos[i]));
		}
	}

void Whip::grabRelease(Body::GrabID grabId)
	{
	/* Bail out if the grab does not exist: */
	GrabMap::Iterator gIt=grabs.findEntry(grabId);
	if(gIt.isFinished())
		return;
	
	/* Release the grab: */
	Grab& grab=gIt->getDest();
	if(grab.grabbedSegment<numParticles-1)
		{
		/* Reset the grabbed particles' masses: */
		for(unsigned int i=0;i<2;++i)
			{
			Index particleIndex=particleIndices[grab.grabbedSegment+i];
			particles.setParticleInvMass(particleIndex,grab.grabbedSegment+i>=2?Scalar(1)/Scalar(0.1):Scalar(1)/Scalar(0.5));
			}
		}
	grabs.removeEntry(gIt);
	}

void Whip::applyForces(Scalar dt,Scalar dt2)
	{
	#if 0
	Scalar forceFactor(10000);
	
	/* Add a small restoring force to each interior particle to stiffen the whip: */
	const Point* p0=&particles.getParticlePosition(particleIndices[0]);
	const Point* p1=&particles.getParticlePosition(particleIndices[1]);
	Vector d0=*p1-*p0;
	for(unsigned int i=2;i<numParticles;++i)
		{
		/* Get the positions of the particle and its neighbors: */
		const Point* p2=&particles.getParticlePosition(particleIndices[i]);
		Vector d1=*p2-*p1;
		Vector n=d1^d0;
		Vector f0=n^d0;
		Vector f1=n^d1;
		particles.forceParticle(particleIndices[i-2],f0,dt2*forceFactor);
		particles.forceParticle(particleIndices[i-1],f0+f1,-dt2*forceFactor);
		particles.forceParticle(particleIndices[i],f1,dt2*forceFactor);
		
		/* Go to the next segment: */
		p0=p1;
		d0=d1;
		p1=p2;
		}
	#endif
	}

void Whip::update(Scalar dt)
	{
	}

void Whip::glRenderAction(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(3.0f);
	
	/* Calculate the total length of the whip and compare it to the ideal length: */
	Scalar whipLength(0);
	const Point* p0=&particles.getParticlePosition(particleIndices[0]);
	for(unsigned int i=1;i<numParticles;++i)
		{
		const Point* p1=&particles.getParticlePosition(particleIndices[i]);
		whipLength+=Geometry::dist(*p0,*p1);
		
		/* Go to the next segment: */
		p0=p1;
		}
	
	GLfloat ratio=GLfloat(whipLength-Scalar(2.65))*Scalar(10)/Scalar(2.65);
	
	/* Render the whip's handle: */
	glBegin(GL_LINES);
	glColor3f(1.0f,0.0f,0.0f);
	glVertex(particles.getParticlePosition(particleIndices[0]));
	glVertex(particles.getParticlePosition(particleIndices[1]));
	glEnd();
	
	/* Render the whip: */
	glBegin(GL_LINE_STRIP);
	
	// glColor3f(0.5f,0.3f,0.2f);
	glColor3f(ratio,1.0f,-ratio);
	
	for(unsigned int i=1;i<numParticles;++i)
		glVertex(particles.getParticlePosition(particleIndices[i]));
	glEnd();
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}
