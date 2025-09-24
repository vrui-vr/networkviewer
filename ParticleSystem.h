/***********************************************************************
ParticleSystem - Class describing a set of moving particles and
constraints on the motion of those particles.
Copyright (c) 2018-2023 Oliver Kreylos

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

#ifndef PARTICLESYSTEM_INCLUDED
#define PARTICLESYSTEM_INCLUDED

#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

#include "ParticleTypes.h"
#include "ParticleOctree.h"

/* Forward declarations: */
namespace Threads {
class Barrier;
}

class ParticleSystem
	{
	/* Embedded classes: */
	public:
	struct DistConstraint // Structure to represent a distance constraint between two particles
		{
		/* Elements: */
		Index index0,index1; // Indices of the two constrained particles
		Scalar dist,dist2; // Distance and squared distance between the two constrained particles
		Scalar strength; // Strength of enforcing the distance constraint, in [0, 1]
		};
	
	struct BoxConstraint // Structure to represent a box constraint
		{
		/* Elements: */
		public:
		bool inside; // Flag whether particles must be inside or outside of this box
		Point min,max; // Minimum and maximum box points
		};
	
	struct SphereConstraint // Structure to represent a sphere constraint
		{
		/* Elements: */
		public:
		bool inside; // Flag whether particles must be inside or outside of this sphere
		Point center; // Center point of sphere
		Scalar radius,radius2; // Radius and squared radius of sphere
		};
	
	class EnforceMinDistFunctor; // Particle octree traversal class to enforce a minimum distance between any pair of particles
	
	/* Elements: */
	private:
	std::vector<DistConstraint> distConstraints; // List of distance constraints between pairs of particles
	Scalar minParticleDist,minParticleDist2; // Minimum distance between any pair of particles and its square
	std::vector<BoxConstraint> boxConstraints; // List of box constraints
	std::vector<SphereConstraint> sphereConstraints; // List of sphere constraints
	Vector gravity; // The particle system's gravity vector
	Scalar attenuation; // Fraction of particle velocities lost per second of simulation time
	Scalar bounce; // Bounce factor for impact between particles and boundary constraints
	Scalar friction; // Friction coefficient for contact between particles and boundary constraints
	Scalar distConstraintScale; // Overall scale factor for the distance constraint solver
	unsigned int numRelaxationIterations; // Number of iterations for the relaxation constraint solver
	Index numParticles; // Number of particles in the system
	std::vector<Scalar> invMass; // Array of inverse particle masses
	std::vector<unsigned int> numDistConstraints; // Number of distance constraints each particle is part of, to keep distance constraint relaxation stable
	std::vector<Point> pos; // Array of current particle positions
	ParticleOctree octree; // Dynamic octree of particles for fast neighborhood searches
	std::vector<Point> prevPos; // Array of particle positions at the previous time step
	Scalar prevDt; // Length of the previous time step
	unsigned int numThreads; // Number of threads from which the particle system's state update methods will be called in parallel
	Threads::Barrier* barrier; // Barrier to synchronize between multiple worker threads
	Vector* particleDeltas; // Array holding particle position update vectors for each thread inside the enforceConstraints method
	
	/* Constructors and destructors: */
	public:
	ParticleSystem(void); // Creates an empty particle system
	~ParticleSystem(void);
	
	/* Methods: */
	void addDistConstraint(Index index0,Index index1,Scalar dist,Scalar strength =Scalar(1)); // Adds a distance constraint to the particle system
	Index getNumDistConstraints(void) const // Returns the number of distance constraints
		{
		return Index(distConstraints.size());
		}
	void setDistConstraintStrength(Index distConstraintIndex,Scalar newStrength); // Sets the strength of the given distance constraint
	Scalar getMinParticleDist(void) const // Returns the minimum distance between any pair of particles
		{
		return minParticleDist;
		}
	void setMinParticleDist(Scalar newMinParticleDist); // Sets a new minimum distance between any pair of particles
	void addBoxConstraint(bool inside,const Point& min,const Point& max); // Adds a box constraint to the particle system
	void addSphereConstraint(bool inside,const Point& center,Scalar radius); // Adds a sphere constraint to the particle system
	const Vector& getGravity(void) const // Returns the particle system's gravity vector
		{
		return gravity;
		}
	void setGravity(const Vector& newGravity); // Sets the particle system's gravity vector
	Scalar getAttenuation(void) const // Returns the particle system's attenuation factor
		{
		return attenuation;
		}
	void setAttenuation(Scalar newAttenuation); // Sets the particle system's attenuation factor
	Scalar getBounce(void) const // Returns the bounce factor for boundary constraint impacts
		{
		return bounce;
		}
	void setBounce(Scalar newBounce); // Sets a new bounce factor for boundary constraint impacts
	Scalar getFriction(void) const // Returns the global friction coefficient
		{
		return friction;
		}
	void setFriction(Scalar newFriction); // Sets a new global friction coefficient
	Scalar getDistConstraintScale(void) const // Returns the distance constraint scale
		{
		return distConstraintScale;
		}
	void setDistConstraintScale(Scalar newDistConstraintScale); // Sets a new distance constraint scale
	unsigned int getNumRelaxationIterations(void) const // Returns the number of constraint enforcement relaxation iterations
		{
		return numRelaxationIterations;
		}
	void setNumRelaxationIterations(unsigned int newNumRelaxationIterations); // Sets the number of iterations for the relaxation of constraints
	unsigned int getNumThreads(void) const // Returns the number of threads from which the state update methods will be called in parallel
		{
		return numThreads;
		}
	void setNumThreads(unsigned int newNumThreads,Threads::Barrier& newBarrier); // Sets the number of threads from which the state update methods will be called in parallel
	Index addParticle(Scalar newInvMass,const Point& newPosition,const Vector& newVelocity); // Adds a particle of the given inverse mass at the given position and with the given initial velocity; returns index of new particle
	void finishUpdate(void); // Finalizes the particle system after particles have been added
	Index getNumParticles(void) const // Returns the number of particles in the system
		{
		return numParticles;
		}
	Scalar getParticleInvMass(Index index) const // Returns a particle's inverse mass
		{
		return invMass[index];
		}
	const Point& getParticlePosition(Index index) const // Returns a particle's position
		{
		return pos[index];
		}
	Point& getParticlePosition(Index index) // Ditto
		{
		return pos[index];
		}
	void setParticleInvMass(Index index,Scalar newInvMass) // Sets a particle's inverse mass
		{
		invMass[index]=newInvMass;
		}
	void setParticlePosition(Index index,const Point& newPosition) // Sets a particle's current position
		{
		pos[index]=newPosition;
		}
	void setParticleVelocity(Index index,const Vector& newVelocity) // Sets a particle's current velocity
		{
		prevPos[index]=pos[index]-newVelocity*prevDt;
		}
	void moveParticles(Scalar dt,unsigned int threadIndex =0); // First part of advance method
	void accelerateParticle(Index index,const Vector& acceleration,Scalar dt2) // Accelerates the given particle with the given acceleration vector over the given squared time step
		{
		/* Move the particle's new position by the given acceleration vector, scaled by squared time step: */
		prevPos[index]+=acceleration*dt2;
		}
	void forceParticle(Index index,const Vector& force,Scalar dt2) // Accelerates the given particle with the given force vector over the given squared time step
		{
		/* Move the particle's new position by the given acceleration vector, scaled by squared time step: */
		prevPos[index]+=force*(invMass[index]*dt2);
		}
	void enforceConstraints(Scalar dt,unsigned int threadIndex =0); // Second step of advance method; accelerate particles between steps 1 and 2
	void advance(Scalar dt,unsigned int threadIndex =0) // Shortcut to advance particle system's state by the given time step without applying forces
		{
		moveParticles(dt,threadIndex);
		enforceConstraints(dt,threadIndex);
		}
	const ParticleOctree& getOctree(void) const // Returns the particle system's octree
		{
		return octree;
		}
	template <class ProcessCloseParticlesFunctor>
	void processCloseParticles(ProcessCloseParticlesFunctor& functor) const // Processes particles close to a given position with the given functor, in approximate order of increasing distance; see traversal functor declaration below
		{
		/* Call the octree's method: */
		octree.processCloseParticles(functor);
		}
	void glRenderAction(bool transparent) const; // Renders the particle system's transparent or non-transparent non-particle state
	};

#endif
