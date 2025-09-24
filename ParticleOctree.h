/***********************************************************************
ParticleOctree - Class to sort particles from a particle system into an
adaptive octree for fast neighborhood queries and close particle
interaction.
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

#ifndef PARTICLEOCTREE_INCLUDED
#define PARTICLEOCTREE_INCLUDED

#include "ParticleTypes.h"

#define PARTICLEOCTREE_BARNES_HUT 1 // Flag whether octree contains a center of gravity for each node for accelerated computation of n-body forces

/* Forward declarations: */
class ParticleSystem;

class ParticleOctree
	{
	private:
	class Node; // Structure representing octree nodes
	
	/* Elements: */
	private:
	const ParticleSystem& particles; // Particle system containing the particles sorted into this octree
	Node* root; // Pointer to the octree's root node
	
	/* Private methods: */
	void tryShrink(void); // Tries shrinking the octree by replacing the current root with its sole used child
	
	/* Constructors and destructors: */
	public:
	static void setMaxParticlesPerNode(size_t newMaxParticlesPerNode); // Sets the maximum allowed number of particles per octree leaf node; must not be called after any octrees have been created
	ParticleOctree(const ParticleSystem& sParticles); // Creates an octree for the given particle system
	private:
	ParticleOctree(const ParticleOctree& source); // Prohibit copy constructor
	ParticleOctree& operator=(const ParticleOctree& source); // Prohibit assignment operator
	public:
	~ParticleOctree(void); // Destroys the particle octree
	
	/* Methods: */
	void addParticle(Index particleIndex); // Adds a new particle of the given system index to the octree
	void removeParticle(Index particleIndex); // Removes the particle of the given system index from the octree
	void finishUpdate(void); // Updates the octree after particles have been added or removed
	template <class ProcessCloseParticlesFunctor>
	void processCloseParticles(ProcessCloseParticlesFunctor& functor) const; // Processes particles close to a given position with the given functor, in approximate order of increasing distance; see traversal functor declaration below
	void updateParticles(void); // Updates the octree after particles have moved due to a simulation step in the particle system
	void glRenderAction(void) const; // Renders the octree's structure into the current OpenGL context
	#if PARTICLEOCTREE_BARNES_HUT
	const Point& getCenterOfGravity(void) const; // Returns the octree's total center of gravity
	template <class ForceAccumulationFunctor>
	void calcForce(ForceAccumulationFunctor& forceAccumulator) const; // Returns the n-body force acting on the given particle with approximation threshold theta
	#endif
	};

#if 0

class ProcessCloseParticlesFunctor // Declaration of functor class compatible with processCloseParticles() method
	{
	/* Methods: */
	public:
	const Point& getCenterPosition(void) const; // Returns the center point around which to search for close-by particles
	Scalar getMaxDist2(void) const; // Returns the squared maximum processing distance
	void operator()(Index particleIndex,const Point& particlePosition,Scalar dist2); // Processes the particle of the given index at the given position and squared distance from the processing center position
	};

class ForceAccumulationFunctor // Declaration of functor class compatible with calcForce() method
	{
	/* Methods: */
	public:
	Index getParticleIndex(void) const; // Returns the index of the particle for which to accumulate forces
	const Point& getParticlePosition(void) const; // Returns the position of the particle for which to accumulate forces
	Scalar getTheta(void) const; // Returns the approximation threshold for the Barnes-Hut algorithm
	void operator()(const Vector& dist,Scalar distLen2,Scalar mass); // Accumulate force from another particle or particle cluster with the given distance vector, squared distance, and particle/cluster mass
	};

#endif

#endif
