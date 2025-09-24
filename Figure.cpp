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

#include "Figure.h"

#include <Misc/ThrowStdErr.h>
#include <IO/ValueSource.h>
#include <IO/OpenFile.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>

#include "ParticleSystem.h"

/***********************
Methods of class Figure:
***********************/

Figure::Figure(ParticleSystem& sParticles,const char* figureFileName,const Body::GrabTransform& initialTransform)
	:Body(sParticles),
	 figureMesh(sParticles),
	 handleRadius(0),
	 grabs(17),nextGrabId(0)
	{
	/* Open the figure definition file: */
	IO::ValueSource figureFile(IO::openFile(figureFileName));
	figureFile.setPunctuation("#\n");
	figureFile.skipWs();
	
	/* Parse the figure definition file line-by-line: */
	Scalar invVertexMass(1);
	Scalar lineStrength(1);
	while(!figureFile.eof())
		{
		/* Get the next token: */
		std::string token=figureFile.readString();
		if(token=="vertex")
			{
			/* Read a vertex definition: */
			Point vertex;
			for(int i=0;i<3;++i)
				vertex[i]=Scalar(figureFile.readNumber());
			
			/* Add the vertex to the particle system: */
			Index particleIndex=particles.addParticle(invVertexMass,initialTransform.transform(vertex),Vector::zero);
			vertexIndices.push_back(particleIndex);
			
			/* Add the vertex to the figure mesh: */
			figureMesh.addVertex(particleIndex);
			}
		else if(token=="line")
			{
			/* Read a distance constraint: */
			Index particleIndices[2];
			for(int i=0;i<2;++i)
				{
				/* Read one vertex index: */
				Index index=Index(figureFile.readUnsignedInteger());
				if(index>=vertexIndices.size())
					Misc::throwStdErr("Figure::Figure: Vertex index %u out of range in line definition in file %s",index,figureFileName);
				
				/* Get the particle index of the requested vertex: */
				particleIndices[i]=vertexIndices[index];
				}
			
			/* Calculate the current distance between the two connected vertices: */
			Scalar particleDist=Geometry::dist(particles.getParticlePosition(particleIndices[0]),particles.getParticlePosition(particleIndices[1]));
			
			/* Add the distance constraint to the particle system: */
			particles.addDistConstraint(particleIndices[0],particleIndices[1],particleDist,lineStrength);
			}
		else if(token=="face")
			{
			/* Read a face definition: */
			Index faceVertexIndices[3];
			for(int i=0;i<3;++i)
				{
				/* Read one vertex index: */
				faceVertexIndices[i]=Index(figureFile.readUnsignedInteger());
				if(faceVertexIndices[i]>=vertexIndices.size())
					Misc::throwStdErr("Figure::Figure: Vertex index %u out of range in face definition in file %s",faceVertexIndices[i],figureFileName);
				}
			
			/* Add a face to the particle mesh: */
			figureMesh.addTriangle(faceVertexIndices[0],faceVertexIndices[1],faceVertexIndices[2]);
			}
		else if(token=="handle")
			{
			/* Read a handle definition: */
			Index particleIndices[2];
			for(int i=0;i<2;++i)
				{
				/* Read one vertex index: */
				Index index=Index(figureFile.readUnsignedInteger());
				if(index>=vertexIndices.size())
					Misc::throwStdErr("Figure::Figure: Vertex index %u out of range in handle definition in file %s",index,figureFileName);
				
				/* Get the particle index of the requested vertex: */
				particleIndices[i]=vertexIndices[index];
				}
			
			/* Add the handle: */
			handles.push_back(Handle(particleIndices));
			}
		else if(token=="handleRadius")
			{
			/* Read the handle radius: */
			handleRadius=Scalar(figureFile.readNumber());
			}
		else if(token=="frontMaterial"||token=="backMaterial")
			{
			/* Read a material definition: */
			GLMaterial::Color ambientDiffuse;
			for(int i=0;i<3;++i)
				ambientDiffuse[i]=Math::clamp(GLfloat(figureFile.readNumber()),0.0f,1.0f);
			GLMaterial::Color specular;
			for(int i=0;i<3;++i)
				specular[i]=Math::clamp(GLfloat(figureFile.readNumber()),0.0f,1.0f);
			GLfloat shininess=Math::max(GLfloat(figureFile.readNumber()),0.0f);
			if(token=="frontMaterial")
				figureMesh.setFrontMaterial(GLMaterial(ambientDiffuse,specular,shininess));
			else
				figureMesh.setBackMaterial(GLMaterial(ambientDiffuse,specular,shininess));
			}
		else if(token!="#"&&token!="\n")
			Misc::throwStdErr("Figure::Figure: Invalid token %s in file %s",token.c_str(),figureFileName);
		
		/* Check that the next token is either a comment marker or a newline: */
		if(token!="#"&&token!="\n"&&figureFile.peekc()!='#'&&figureFile.peekc()!='\n')
			Misc::throwStdErr("Figure::Figure: Extra tokens at end of line in file %s",figureFileName);
			
		/* Skip the rest of the line: */
		if(token!="\n")
			figureFile.skipLine();
		figureFile.skipWs();
		}
	}

Figure::~Figure(void)
	{
	}

Body::GrabID Figure::grab(const Point& grabPos,Scalar grabRadius,const Body::GrabTransform& initialGrabTransform)
	{
	/* Check all figure handles against the grab position: */
	unsigned int handleIndex=0;
	for(std::vector<Handle>::iterator hIt=handles.begin();hIt!=handles.end();++hIt,++handleIndex)
		{
		/* Get the positions of the handle's two end particles: */
		Point p0=particles.getParticlePosition((*hIt)[0]);
		Point p1=particles.getParticlePosition((*hIt)[1]);
		
		/* Check if the grab position is inside the handle's cylinder: */
		Vector d=p1-p0;
		Scalar dLen2=d.sqr();
		Vector pgp=grabPos-p0;
		Scalar dd=pgp*d;
		pgp-=d*(dd/dLen2); 
		if(pgp.sqr()<=Math::sqr(handleRadius+grabRadius))
			{
			Scalar dLen=Math::sqrt(dLen2);
			dd/=dLen;
			if(dd>=-(handleRadius+grabRadius)&&dd<=dLen+handleRadius+grabRadius)
				{
				/* Found the grabbed handle: */
				break;
				}
			}
		}
	
	/* Bail out if no handle was grabbed: */
	if(handleIndex>=handles.size())
		return GrabID(0);
	
	/* Check if the grabbed segment was already grabbed: */
	for(GrabMap::Iterator gmIt=grabs.begin();!gmIt.isFinished();++gmIt)
		{
		Grab& grab=gmIt->getDest();
		if(grab.grabbedHandleIndex==handleIndex)
			{
			/* Release the handle from the existing grab and stop looking: */
			for(int i=0;i<2;++i)
				{
				Index particleIndex=handles[grab.grabbedHandleIndex][i];
				particles.setParticleInvMass(particleIndex,grab.grabbedParticleInvMass[i]);
				}
			grab.grabbedHandleIndex=handles.size();
			break;
			}
		}
	
	/* Create a new grab: */
	Grab newGrab;
	newGrab.grabbedHandleIndex=handleIndex;
	Handle& handle=handles[handleIndex];
	for(int i=0;i<2;++i)
		{
		Index particleIndex=handle[i];
		newGrab.grabbedParticlePos[i]=initialGrabTransform.inverseTransform(particles.getParticlePosition(particleIndex));
		newGrab.grabbedParticleInvMass[i]=particles.getParticleInvMass(particleIndex);
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

void Figure::grabUpdate(Body::GrabID grabId,const Body::GrabTransform& newGrabTransform)
	{
	/* Bail out if the grab does not exist or is invalid: */
	GrabMap::Iterator gIt=grabs.findEntry(grabId);
	if(gIt.isFinished()||gIt->getDest().grabbedHandleIndex>=handles.size())
		return;
	
	/* Update the positions of the particles at the two ends of the grabbed handle: */
	Grab& grab=gIt->getDest();
	for(unsigned int i=0;i<2;++i)
		{
		Index particleIndex=handles[grab.grabbedHandleIndex][i];
		particles.setParticlePosition(particleIndex,newGrabTransform.transform(grab.grabbedParticlePos[i]));
		}
	}

void Figure::grabRelease(Body::GrabID grabId)
	{
	/* Bail out if the grab does not exist: */
	GrabMap::Iterator gIt=grabs.findEntry(grabId);
	if(gIt.isFinished())
		return;
	
	/* Release the grab: */
	Grab& grab=gIt->getDest();
	if(grab.grabbedHandleIndex<handles.size())
		{
		/* Reset the grabbed particles' masses: */
		for(unsigned int i=0;i<2;++i)
			{
			Index particleIndex=handles[grab.grabbedHandleIndex][i];
			particles.setParticleInvMass(particleIndex,grab.grabbedParticleInvMass[i]);
			}
		}
	grabs.removeEntry(gIt);
	}

void Figure::update(Scalar dt)
	{
	/* Update the particle mesh: */
	figureMesh.update();
	}

void Figure::glRenderAction(GLContextData& contextData) const
	{
	/* Draw the figure mesh: */
	figureMesh.glRenderAction(contextData);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(3.0f);
	
	/* Render all handles: */
	glBegin(GL_LINES);
	glColor3f(1.0f,0.0f,0.0f);
	for(std::vector<Handle>::const_iterator hIt=handles.begin();hIt!=handles.end();++hIt)
		{
		glVertex(particles.getParticlePosition((*hIt)[0]));
		glVertex(particles.getParticlePosition((*hIt)[1]));
		}
	glEnd();
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}
