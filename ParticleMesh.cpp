/***********************************************************************
ParticleMesh - Class to represent triangle meshes whose vertices are
particles in a particle system.
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

#define GLGEOMETRY_NONSTANDARD_TEMPLATES

#include "ParticleMesh.h"

#include <GL/GLVertexArrayParts.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>

/***************************************
Methods of class ParticleMesh::DataItem:
***************************************/

ParticleMesh::DataItem::DataItem(void)
	:vertexBuffer(0),indexBuffer(0),
	 vertexVersion(0)
	{
	/* Initialize the GL_ARB_vertex_buffer_object extension: */
	GLARBVertexBufferObject::initExtension();
	
	/* Allocate the vertex and index buffer objects: */
	glGenBuffersARB(1,&vertexBuffer);
	glGenBuffersARB(1,&indexBuffer);
	}

ParticleMesh::DataItem::~DataItem(void)
	{
	/* Release the vertex and index buffer objects: */
	glDeleteBuffersARB(1,&vertexBuffer);
	glDeleteBuffersARB(1,&indexBuffer);
	}

/*****************************
Methods of class ParticleMesh:
*****************************/

ParticleMesh::ParticleMesh(const ParticleSystem& sParticleSystem)
	:particleSystem(sParticleSystem),
	 vertexVersion(0),
	 twoSided(false)
	{
	}

ParticleMesh::~ParticleMesh(void)
	{
	}

void ParticleMesh::initContext(GLContextData& contextData) const
	{
	/* Create an OpenGL context data item and associate it with the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Initialize the vertex buffer: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,vertexIndices.size()*sizeof(Vertex),0,GL_DYNAMIC_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	/* Initialize the index buffer: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBuffer);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,triangleVertexIndices.size()*sizeof(GLuint),&triangleVertexIndices.front(),GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	}

void ParticleMesh::update(void)
	{
	/* Re-calculate per-vertex normal vectors: */
	for(std::vector<Vector>::iterator vnIt=vertexNormals.begin();vnIt!=vertexNormals.end();++vnIt)
		*vnIt=Vector::zero;
	
	/* Calculate the normal vectors of all triangles and accumulate them into the per-vertex normal array: */
	for(std::vector<GLuint>::iterator tviIt=triangleVertexIndices.begin();tviIt!=triangleVertexIndices.end();tviIt+=3)
		{
		/* Get the current positions of the triangle's three vertices from the particle system: */
		const Point& v0=particleSystem.getParticlePosition(vertexIndices[tviIt[0]]);
		const Point& v1=particleSystem.getParticlePosition(vertexIndices[tviIt[1]]);
		const Point& v2=particleSystem.getParticlePosition(vertexIndices[tviIt[2]]);
		
		/* Calculate the triangle's normal vector, scaled by the triangle's area: */
		Vector normal=(v1-v0)^(v2-v0);
		
		/* Add the scaled normal vector to the three vertices' per-vertex normal vectors: */
		for(int i=0;i<3;++i)
			vertexNormals[tviIt[i]]+=normal;
		}
	
	/* Invalidate the vertex buffer object: */
	++vertexVersion;
	}

void ParticleMesh::setFrontMaterial(const GLMaterial& newFrontMaterial)
	{
	frontMaterial=newFrontMaterial;
	}

void ParticleMesh::setBackMaterial(const GLMaterial& newBackMaterial)
	{
	twoSided=true;
	backMaterial=newBackMaterial;
	}

void ParticleMesh::glRenderAction(GLContextData& contextData) const
	{
	/* Retrieve the OpenGL context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set OpenGL state: */
	glMaterial(GLMaterialEnums::FRONT,frontMaterial);
	if(twoSided)
		{
		glDisable(GL_CULL_FACE);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
		glMaterial(GLMaterialEnums::BACK,backMaterial);
		}
	
	/* Bind the vertex buffer: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBuffer);
	
	/* Check if the vertex buffer is outdated: */
	if(dataItem->vertexVersion!=vertexVersion)
		{
		/* Copy current vertex positions and normal vectors into the buffer: */
		Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		std::vector<Vector>::const_iterator vnIt=vertexNormals.begin();
		for(std::vector<Index>::const_iterator viIt=vertexIndices.begin();viIt!=vertexIndices.end();++viIt,++vnIt,++vPtr)
			{
			/* Copy the vertex's normal vector: */
			vPtr->normal=*vnIt;
			
			/* Copy the vertex's position from the particle system: */
			vPtr->position=particleSystem.getParticlePosition(*viIt);
			}
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		/* Mark the vertex buffer as up-to-date: */
		dataItem->vertexVersion=vertexVersion;
		}
	
	/* Set up vertex array rendering: */
	GLVertexArrayParts::enable(Vertex::getPartsMask());
	glVertexPointer(static_cast<const Vertex*>(0));
	
	/* Bind the index buffer: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBuffer);
	
	/* Draw the mesh: */
	glDrawElements(GL_TRIANGLES,triangleVertexIndices.size(),GL_UNSIGNED_INT,0);
	
	/* Disable vertex array rendering: */
	GLVertexArrayParts::disable(Vertex::getPartsMask());
	
	/* Unbind the vertex and index buffers: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	if(twoSided)
		{
		glEnable(GL_CULL_FACE);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
		}
	}
