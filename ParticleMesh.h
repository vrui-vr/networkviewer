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

#ifndef PARTICLEMESH_INCLUDED
#define PARTICLEMESH_INCLUDED

#include <vector>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GL/GLGeometryVertex.h>

#include "ParticleSystem.h"

class ParticleMesh:public GLObject
	{
	private:
	typedef GLGeometry::Vertex<void,0,void,0,Scalar,Scalar,3> Vertex; // Type for vertices stored in OpenGL buffers
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBuffer; // ID of buffer holding mesh vertices
		GLuint indexBuffer; // ID of buffer holding mesh vertex indices
		unsigned int vertexVersion; // Version number of data in vertex buffer
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	const ParticleSystem& particleSystem; // Reference to the particle system controlling the mesh vertices
	std::vector<Index> vertexIndices; // List of indices of particles used as mesh vertices
	std::vector<GLuint> triangleVertexIndices; // List of indices of vertices in the vertexIndices list defining mesh triangles
	std::vector<Vector> vertexNormals; // List of per-vertex normal vectors
	unsigned int vertexVersion; // Version number of mesh vertices
	GLMaterial frontMaterial; // Rendering material properties for front faces
	bool twoSided; // Flag whether this mesh is rendered from both sides
	GLMaterial backMaterial; // Rendering material properties for back faces if material is two-sided
	
	/* Constructors and destructors: */
	public:
	ParticleMesh(const ParticleSystem& sParticleSystem); // Creates an empty particle mesh using particles from the given particle system
	~ParticleMesh(void); // Destroys the particle mesh
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	void addVertex(Index newIndex) // Adds a new vertex to the mesh
		{
		vertexIndices.push_back(newIndex);
		vertexNormals.push_back(Vector::zero);
		}
	void addTriangle(GLuint index0,GLuint index1,GLuint index2) // Adds a new triangle to the mesh
		{
		triangleVertexIndices.push_back(index0);
		triangleVertexIndices.push_back(index1);
		triangleVertexIndices.push_back(index2);
		}
	void addTriangle(const GLuint indices[3]) // Ditto
		{
		for(int i=0;i<3;++i)
			triangleVertexIndices.push_back(indices[i]);
		}
	Index getVertexIndex(unsigned int index) const // Returns the particle index of the mesh vertex of the given index
		{
		return vertexIndices[index];
		}
	void update(void); // Updates the mesh with new particle positions from the given particle system
	void setFrontMaterial(const GLMaterial& newFrontMaterial); // Sets the mesh's front face material
	void setBackMaterial(const GLMaterial& newBackMaterial); // Sets the mesh's back face material and enables two-sided rendering
	void glRenderAction(GLContextData& contextData) const; // Renders the particle mesh into the given OpenGL context
	};

#endif
