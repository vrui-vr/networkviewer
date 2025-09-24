/***********************************************************************
ForceFunctors - Functor classes to calculate particle interactions in a
network simulation.
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

#ifndef FORCEFUNCTORS_INCLUDED
#define FORCEFUNCTORS_INCLUDED

#include <Geometry/Random.h>

#include "ParticleTypes.h"
#include "ParticleSystem.h"

/***********************************************************************
Class for repulsive forces with a finite cut-off distance, used during
particle octree traversal.
***********************************************************************/

class LocalRepulsiveForceFunctor
	{
	/* Elements: */
	private:
	ParticleSystem& particles; // Reference to particle system
	Index index; // Index of source particle
	const Point& position; // Source particle's position
	Scalar dt2; // Squared time step
	
	/* Constructors and destructors: */
	public:
	LocalRepulsiveForceFunctor(ParticleSystem& sParticles,Index sIndex,Scalar sDt2)
		:particles(sParticles),
		 index(sIndex),position(particles.getParticlePosition(index)),
		 dt2(sDt2)
		{
		}
	
	/* Methods: */
	const Point& getCenterPosition(void) const
		{
		return position;
		}
	Scalar getMaxDist2(void) const
		{
		return Scalar(4); // Two times link length, squared
		}
	void operator()(Index particleIndex,const Point& particlePosition,Scalar dist2)
		{
		if(index<particleIndex)
			{
			/* Calculate the force: */
			if(dist2>Scalar(0))
				{
				/* Calculate force vector: */
				Vector force=(particles.getParticlePosition(particleIndex)-position)*((Scalar(10)-Scalar(5)*Math::sqrt(dist2))/dist2);
				
				/* Apply the force vector to both particles: */
				particles.forceParticle(index,-force,dt2);
				particles.forceParticle(particleIndex,force,dt2);
				}
			else
				{
				/* Do something clever... */
				}
			}
		}
	};

/***********************************************************************
Base class for global (n-body) repulsive forces.
***********************************************************************/

class GlobalRepulsiveForceFunctor
	{
	/* Elements: */
	protected:
	Scalar theta; // Barnes-Hut approximation threshold
	Scalar minDist,minDist2; // Squared minimum particle distance for inverse force law
	Index particleIndex; // Index of particle for which forces are accumulated
	Point particlePosition; // Position of particle for which forces are accumulated
	Vector force; // Accumulated force
	
	/* Constructors and destructors: */
	public:
	GlobalRepulsiveForceFunctor(Scalar sTheta,Scalar sMinDist)
		:theta(sTheta),minDist(sMinDist),minDist2(Math::sqr(minDist))
		{
		}
	
	/* Methods: */
	void prepareParticle(Index newParticleIndex,const Point& newParticlePosition) // Prepares to accumulate force for a new particle
		{
		/* Remember the new particle: */
		particleIndex=newParticleIndex;
		particlePosition=newParticlePosition;
		
		/* Reset the force accumulator: */
		force=Vector::zero;
		}
	Index getParticleIndex(void) const
		{
		return particleIndex;
		}
	const Point& getParticlePosition(void) const
		{
		return particlePosition;
		}
	Scalar getTheta(void) const
		{
		return theta;
		}
	const Vector& getForce(void) const // Returns the accumulated force
		{
		return force;
		}
	};

/***********************************************************************
Class for global (n-body) repulsive forces using an inverse linear law.
***********************************************************************/

class GlobalRepulsiveForceFunctorLinear:public GlobalRepulsiveForceFunctor
	{
	/* Constructors and destructors: */
	public:
	GlobalRepulsiveForceFunctorLinear(Scalar sTheta,Scalar sMinDist)
		:GlobalRepulsiveForceFunctor(sTheta,sMinDist)
		{
		}
	
	/* Methods: */
	void operator()(const Vector& dist,Scalar distLen2,Scalar mass)
		{
		/* Check the distance against the inverse force law cutoff: */
		if(distLen2>=minDist2)
			{
			/* Use inverse force law: */
			force-=dist*(mass/distLen2);
			}
		else if(distLen2>Scalar(0))
			{
			/* Use cut-off inverse force law: */
			force-=dist*(mass/Math::sqrt(minDist2*distLen2));
			}
		else
			{
			/* Use a random distance vector: */
			Vector d=Geometry::randVectorUniform<Scalar,3>(minDist);
			force-=d*(mass/minDist2);
			}
		}
	};

/***********************************************************************
Class for global (n-body) repulsive forces using an inverse square law.
***********************************************************************/

class GlobalRepulsiveForceFunctorQuadratic:public GlobalRepulsiveForceFunctor
	{
	/* Constructors and destructors: */
	public:
	GlobalRepulsiveForceFunctorQuadratic(Scalar sTheta,Scalar sMinDist)
		:GlobalRepulsiveForceFunctor(sTheta,sMinDist)
		{
		}
	
	/* Methods: */
	void operator()(const Vector& dist,Scalar distLen2,Scalar mass)
		{
		/* Check the distance against the inverse force law cutoff: */
		if(distLen2>=minDist2)
			{
			/* Use inverse force law: */
			force-=dist*(mass/(distLen2*Math::sqrt(distLen2)));
			}
		else if(distLen2>Scalar(0))
			{
			/* Use cut-off inverse force law: */
			force-=dist*(mass/(minDist2*Math::sqrt(distLen2)));
			}
		else
			{
			/* Use a random distance vector: */
			Vector d=Geometry::randVectorUniform<Scalar,3>(minDist);
			force-=d*(mass/(minDist2*minDist));
			}
		}
	};

#endif
