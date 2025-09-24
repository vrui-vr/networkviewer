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

#include "ParticleSystem.h"

#include <utility>
#include <stdexcept>
#include <Threads/Barrier.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Math/Random.h>
#include <GL/gl.h>
#include <GL/GLNormalTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLModels.h>

#include "ParticleOctree.icpp"

/**********************************************************
Declaration of class ParticleSystem::EnforceMinDistFunctor:
**********************************************************/

class ParticleSystem::EnforceMinDistFunctor
	{
	/* Elements: */
	private:
	ParticleSystem& particles; // Particle system being operated on
	Index index; // Index of particle being tested
	Point& position; // Position of particle being tested
	Scalar invMass; // Inverse mass of particle being tested
	Scalar minDist2; // Squared minimum particle distance
	
	/* Constructors and destructors: */
	public:
	EnforceMinDistFunctor(ParticleSystem& sParticles,Index sIndex,Scalar sMinDist2)
		:particles(sParticles),index(sIndex),
		 position(particles.getParticlePosition(index)),
		 invMass(particles.getParticleInvMass(index)),
		 minDist2(sMinDist2)
		{
		}
	
	/* Methods: */
	const Point& getCenterPosition(void) const
		{
		return position;
		}
	Scalar getMaxDist2(void) const
		{
		return minDist2;
		}
	void operator()(Index particleIndex,const Point& particlePosition,Scalar dist2)
		{
		if(index<particleIndex)
			{
			Point& otherPosition=particles.getParticlePosition(particleIndex);
			Vector d=otherPosition-position;
			Scalar otherInvMass=particles.getParticleInvMass(particleIndex);
			Scalar invMassSum=invMass+otherInvMass;
			if(invMassSum>Scalar(0))
				{
				d*=(Scalar(1)-Scalar(2)*minDist2/(d.sqr()+minDist2))/invMassSum;
				position+=d*invMass;
				otherPosition-=d*otherInvMass;
				}
			else
				{
				d*=Scalar(0.5)-minDist2/(d.sqr()+minDist2);
				position+=d;
				otherPosition-=d;
				}
			}
		}
	};

/*******************************
Methods of class ParticleSystem:
*******************************/

ParticleSystem::ParticleSystem(void)
	:minParticleDist(0),minParticleDist2(0),
	 gravity(0.0,0.0,-9.81),
	 attenuation(0.75),bounce(0.0),friction(1.0),
	 distConstraintScale(1.0),numRelaxationIterations(10),
	 numParticles(0),
	 octree(*this),
	 prevDt(1),
	 numThreads(1),barrier(0),particleDeltas(0)
	{
	}

ParticleSystem::~ParticleSystem(void)
	{
	/* Release allocated resources: */
	delete[] particleDeltas;
	}

void ParticleSystem::addDistConstraint(Index index0,Index index1,Scalar dist,Scalar strength)
	{
	/* Store the new constraint: */
	DistConstraint dc;
	dc.index0=index0;
	dc.index1=index1;
	dc.dist=dist;
	dc.dist2=Math::sqr(dist);
	dc.strength=strength;
	distConstraints.push_back(dc);
	
	/* Update the involved particles' distance constraint counters: */
	++numDistConstraints[index0];
	++numDistConstraints[index1];
	}

void ParticleSystem::setDistConstraintStrength(Index distConstraintIndex,Scalar newStrength)
	{
	distConstraints[distConstraintIndex].strength=newStrength;
	}

void ParticleSystem::setMinParticleDist(Scalar newMinParticleDist)
	{
	/* Update the minimum distance and its square: */
	minParticleDist=newMinParticleDist;
	minParticleDist2=Math::sqr(minParticleDist);
	}

void ParticleSystem::addBoxConstraint(bool inside,const Point& min,const Point& max)
	{
	/* Store the new constraint: */
	BoxConstraint bc;
	bc.inside=inside;
	bc.min=min;
	bc.max=max;
	boxConstraints.push_back(bc);
	}

void ParticleSystem::addSphereConstraint(bool inside,const Point& center,Scalar radius)
	{
	/* Store the new constraint: */
	SphereConstraint sc;
	sc.inside=inside;
	sc.center=center;
	sc.radius=radius;
	sc.radius2=Math::sqr(radius);
	sphereConstraints.push_back(sc);
	}

void ParticleSystem::setGravity(const Vector& newGravity)
	{
	gravity=newGravity;
	}

void ParticleSystem::setAttenuation(Scalar newAttenuation)
	{
	attenuation=newAttenuation;
	}

void ParticleSystem::setBounce(Scalar newBounce)
	{
	bounce=newBounce;
	}

void ParticleSystem::setFriction(Scalar newFriction)
	{
	friction=newFriction;
	}

void ParticleSystem::setDistConstraintScale(Scalar newDistConstraintScale)
	{
	distConstraintScale=newDistConstraintScale;
	}

void ParticleSystem::setNumRelaxationIterations(unsigned int newNumRelaxationIterations)
	{
	numRelaxationIterations=newNumRelaxationIterations;
	}

void ParticleSystem::setNumThreads(unsigned int newNumThreads,Threads::Barrier& newBarrier)
	{
	numThreads=newNumThreads;
	barrier=&newBarrier;
	
	/* Re-allocate the particle position update vector array: */
	delete[] particleDeltas;
	particleDeltas=new Vector[size_t(numThreads)*size_t(numParticles)];
	}

Index ParticleSystem::addParticle(Scalar newInvMass,const Point& newPosition,const Vector& newVelocity)
	{
	Index result=numParticles;
	
	/* Store the particle's inverse mass: */
	invMass.push_back(newInvMass);
	
	/* Initialize the particle's distance constraint counter: */
	numDistConstraints.push_back(0);
	
	/* Store the particle's initial position: */
	pos.push_back(newPosition);
	
	/* Store the particle's initial velocity by calculating its phantom position on the previous time step: */
	prevPos.push_back(newPosition-newVelocity*prevDt);
	
	/* Add the particle to the particle octree: */
	octree.addParticle(result);
	
	/* Increment the number of particles and return the new index: */
	++numParticles;
	return result;
	}

void ParticleSystem::finishUpdate(void)
	{
	/* Finish the particle octree: */
	octree.finishUpdate();
	
	/* Allocate temporary storage: */
	particleDeltas=new Vector[size_t(numThreads)*size_t(numParticles)];
	}

void ParticleSystem::moveParticles(Scalar dt,unsigned int threadIndex)
	{
	/* Calculate the Verlet integration coefficients: */
	Scalar att=Math::pow(attenuation,prevDt);
	Scalar pc=dt*att/prevDt;
	Scalar c=Scalar(1)+pc;
	Scalar dt2=Math::sqr(dt);
	
	/* Calculate gravity acceleration vectors for position and velocity updates: */
	Vector g=gravity*dt2; // Corresponds to Euler integration where velocity is updated before position
	// Vector g=gravity*(Scalar(0.5)*dt2); // This would be g for a quadratic approximation, but there are problems with the octree
	
	/* Update the positions of all particles handled by this thread: */
	std::vector<Point>::iterator ppEnd=prevPos.begin()+((threadIndex+1)*numParticles)/numThreads;
	std::vector<Point>::iterator pIt=pos.begin()+(threadIndex*numParticles)/numThreads;
	for(std::vector<Point>::iterator ppIt=prevPos.begin()+(threadIndex*numParticles)/numThreads;ppIt!=ppEnd;++ppIt,++pIt)
		for(int i=0;i<3;++i)
			{
			// (*ppIt)[i]=(*pIt)[i]*c-(*ppIt)[i]*pc+g[i];
			(*ppIt)[i]=(*pIt)[i]+((*pIt)[i]-(*ppIt)[i])*pc+g[i];
			// (*pIt)[i]-=g[i]; // Would have to do this for quadratic approximation, but it moves a particle's previous position and violates octree constraints
			}
	}

void ParticleSystem::enforceConstraints(Scalar dt,unsigned int threadIndex)
	{
	/* Synchronize between all threads and let one thread swap the particle position arrays, then synchronize again: */
	bool swapPositions=true;
	if(barrier!=0)
		swapPositions=barrier->synchronize();
	if(swapPositions)
		{
		/* Swap previous and current particle positions: */
		std::swap(prevPos,pos);
		prevDt=dt;
		}
	if(barrier!=0)
		barrier->synchronize();
	
	/* Set the range of particles on which this thread will operate: */
	Index iBegin((threadIndex*numParticles)/numThreads);
	Index iEnd(((threadIndex+1)*numParticles)/numThreads);
	std::vector<Point>::iterator pBegin=pos.begin()+iBegin;
	std::vector<Point>::iterator pEnd=pos.begin()+iEnd;
	std::vector<Point>::iterator ppBegin=prevPos.begin()+iBegin;
	
	/* Enforce boundary constraints (box and sphere) first to handle bounce and friction: */
	for(std::vector<BoxConstraint>::iterator bcIt=boxConstraints.begin();bcIt!=boxConstraints.end();++bcIt)
		{
		/* Check the type of box constraint: */
		if(bcIt->inside)
			{
			/* Keep all particles inside of the box: */
			std::vector<Point>::iterator ppIt=ppBegin;
			for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt,++ppIt)
				for(int i=0;i<3;++i)
					{
					if((*pIt)[i]<bcIt->min[i])
						{
						/* Calculate particle's penetration depth for bounce and friction: */
						Scalar d=bcIt->min[i]-(*pIt)[i];
						
						/* Calculate particle's bounce: */
						(*pIt)[i]=bcIt->min[i]+d*bounce;
						(*ppIt)[i]=bcIt->min[i]+(bcIt->min[i]-(*ppIt)[i])*bounce;
						
						/* Calculate particle's friction: */
						Vector v=*pIt-*ppIt;
						v[i]=Scalar(0);
						Scalar vLen=v.mag();
						Scalar fLen=friction*d;
						if(vLen>fLen)
							*pIt-=v*(fLen/vLen);
						else
							*pIt-=v;
						}
					else if((*pIt)[i]>bcIt->max[i])
						{
						/* Calculate particle's penetration depth for bounce and friction: */
						Scalar d=(*pIt)[i]-bcIt->max[i];
						
						/* Calculate particle's bounce: */
						(*pIt)[i]=bcIt->max[i]-d*bounce;
						(*ppIt)[i]=bcIt->max[i]+(bcIt->max[i]-(*ppIt)[i])*bounce;
						
						/* Calculate particle's friction: */
						Vector v=*pIt-*ppIt;
						v[i]=Scalar(0);
						Scalar vLen2=v.sqr();
						Scalar fLen=friction*d;
						if(vLen2>Math::sqr(fLen))
							*pIt-=v*(fLen/Math::sqrt(vLen2));
						else
							*pIt-=v;
						}
					}
			}
		else
			{
			/* Keep all particles outside of the box: */
			std::vector<Point>::iterator ppIt=ppBegin;
			for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt,++ppIt)
				{
				/* Calculate the particle path's intersection interval with the box: */
				Scalar minLambda(0);
				Scalar maxLambda(1);
				int minAxis=-1;
				Scalar minBound(0);
				for(int i=0;i<3;++i)
					{
					Scalar po=(*ppIt)[i];
					Scalar p=(*pIt)[i];
					Scalar min=bcIt->min[i];
					Scalar max=bcIt->max[i];
					if(po<=min)
						{
						if(p>min)
							{
							Scalar l0=(min-po)/(p-po);
							if(minLambda<=l0)
								{
								minLambda=l0;
								minAxis=i;
								minBound=min;
								}
							if(p>max)
								{
								Scalar l1=(max-po)/(p-po);
								if(maxLambda>l1)
									maxLambda=l1;
								}
							}
						else
							minLambda=maxLambda;
						}
					else if(po>=max)
						{
						if(p<max)
							{
							Scalar l0=(max-po)/(p-po);
							if(minLambda<=l0)
								{
								minLambda=l0;
								minAxis=i;
								minBound=max;
								}
							if(p<min)
								{
								Scalar l1=(min-po)/(p-po);
								if(maxLambda>l1)
									maxLambda=l1;
								}
							}
						else
							minLambda=maxLambda;
						}
					else
						{
						if(p<min)
							{
							Scalar l1=(min-po)/(p-po);
							if(maxLambda>l1)
								maxLambda=l1;
							}
						else if(p>max)
							{
							Scalar l1=(max-po)/(p-po);
							if(maxLambda>l1)
								maxLambda=l1;
							}
						}
					}
				
				/* Check if there was an intersection: */
				if(minLambda<maxLambda)
					{
					/* Calculate particle's penetration depth: */
					Scalar d=(*pIt)[minAxis]-minBound;
					
					/* Calculate particle's bounce: */
					(*pIt)[minAxis]=minBound-d*bounce;
					(*ppIt)[minAxis]=minBound-((*ppIt)[minAxis]-minBound)*bounce;
					
					/* Calculate particle's friction: */
					Vector v=*pIt-*ppIt;
					v[minAxis]=Scalar(0);
					Scalar vLen2=v.sqr();
					Scalar fLen=friction*Math::abs(d);
					if(vLen2>Math::sqr(fLen))
						*pIt-=v*(fLen/Math::sqrt(vLen2));
					else
						*pIt-=v;
					}
				}
			}
		}
	for(std::vector<SphereConstraint>::iterator scIt=sphereConstraints.begin();scIt!=sphereConstraints.end();++scIt)
		{
		/* Check the type of sphere constraint: */
		if(scIt->inside)
			{
			/* Keep all particles inside of the sphere: */
			std::vector<Point>::iterator ppIt=ppBegin;
			for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt,++ppIt)
				{
				Scalar dist2=Geometry::sqrDist(*pIt,scIt->center);
				if(dist2>scIt->radius2)
					{
					/* Find the second intersection of the particle's path with the sphere (the one where the particle exits the sphere): */
					Vector poc=*ppIt-scIt->center;
					Vector ppo=*pIt-*ppIt;
					Scalar a=ppo.sqr();
					Scalar b=Scalar(2)*(poc*ppo);
					Scalar c=poc.sqr()-scIt->radius2;
					
					/* Find the larger solution of the quadratic equation a*x^2 + b*x + c = 0: */
					Scalar sq=Math::sqrt(Math::sqr(b)-Scalar(4)*a*c);
					Scalar lambda=b>=Scalar(0)?(Scalar(2)*c)/(-b-sq):(-b+sq)/(Scalar(2)*a);
					
					/* Calculate the particle's contact point: */
					Point cp=*ppIt+ppo*lambda;
					
					/* Calculate the vector that projects the particle onto the contact point's tangent plane: */
					Vector n=cp-scIt->center;
					Vector bounceVec=n*((ppo*n)/scIt->radius2); // scIt->radius2 is the squared length of n
					
					/* Reflect the particle's position about the tangent plane: */
					(*pIt)-=bounceVec*((Scalar(1)-lambda)*(Scalar(1)+bounce));
					
					/* Reflect the particle's velocity about the tangent plane: */
					(*ppIt)+=bounceVec*(lambda*(Scalar(1)+bounce));
					
					/* Project the particle's velocity vector into the tangent plane to apply friction: */
					ppo-=bounceVec;
					Scalar vLen2=ppo.sqr();
					Scalar fLen=friction*(Math::sqrt(dist2)-scIt->radius);
					if(vLen2>Math::sqr(fLen))
						*pIt-=ppo*(fLen/Math::sqrt(vLen2));
					else
						*pIt-=ppo;
					}
				}
			}
		else
			{
			/* Keep all particles outside of the sphere: */
			std::vector<Point>::iterator ppIt=ppBegin;
			for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt,++ppIt)
				{
				/* Find the first intersection of the particle's path with the sphere (the one where the particle enters the sphere): */
				Vector poc=*ppIt-scIt->center;
				Vector ppo=*pIt-*ppIt;
				Scalar a=ppo.sqr();
				Scalar b=Scalar(2)*(poc*ppo);
				Scalar c=poc.sqr()-scIt->radius2;
				
				/* Find the smaller solution of the quadratic equation a*x^2 + b*x + c = 0: */
				Scalar sq=Math::sqr(b)-Scalar(4)*a*c;
				if(sq>=Scalar(0))
					{
					sq=Math::sqrt(sq);
					Scalar lambda=b>=Scalar(0)?(-b-sq)/(Scalar(2)*a):(Scalar(2)*c)/(-b+sq);
					if(lambda>=Scalar(-1.0e-1)&&lambda<Scalar(1))
						{
						/* Calculate the particle's contact point: */
						Point cp=*ppIt+ppo*lambda;
						
						/* Calculate the vector that projects the particle onto the contact point's tangent plane: */
						Vector n=cp-scIt->center;
						Vector bounceVec=n*((ppo*n)/scIt->radius2); // scIt->radius2 is the squared length of n
						
						/* Reflect the particle's position about the tangent plane: */
						(*pIt)-=bounceVec*((Scalar(1)-lambda)*(Scalar(1)+bounce));
						
						/* Reflect the particle's velocity about the tangent plane: */
						(*ppIt)+=bounceVec*(lambda*(Scalar(1)+bounce));
						
						/* Project the particle's velocity vector into the tangent plane to apply friction: */
						ppo-=bounceVec;
						Scalar vLen2=ppo.sqr();
						Scalar fLen=friction*bounceVec.mag()*(Scalar(1)-lambda);
						if(vLen2>Math::sqr(fLen))
							*pIt-=ppo*(fLen/Math::sqrt(vLen2));
						else
							*pIt-=ppo;
						}
					}
				}
			}
		}
	
	/* Set the range of distance constraints on which this thread operates: */
	std::vector<DistConstraint>::iterator dcBegin=distConstraints.begin()+(threadIndex*distConstraints.size())/numThreads;
	std::vector<DistConstraint>::iterator dcEnd=distConstraints.begin()+((threadIndex+1)*distConstraints.size())/numThreads;
	
	/* Enforce all constraints through iterative relaxation: */
	for(unsigned int iteration=0;iteration<numRelaxationIterations;++iteration)
		{
		/* Synchronize between all threads to switch from per-particle to per-constraint parallelization: */
		if(barrier!=0)
			barrier->synchronize();
		
		/* Reset this thread's particle position update vectors: */
		Vector* pds=particleDeltas+size_t(threadIndex)*size_t(numParticles);
		for(Index i=0;i<numParticles;++i)
			pds[i]=Vector::zero;
		
		/* Process all distance constraints: */
		for(std::vector<DistConstraint>::iterator dcIt=dcBegin;dcIt!=dcEnd;++dcIt)
			{
			/* Get the two particles' inverse masses and their sum: */
			Scalar im0=invMass[dcIt->index0];
			Scalar im1=invMass[dcIt->index1];
			Scalar imSum=im0+im1;
			
			/* Calculate the current distance vector between the two particles and its squared length: */
			Vector d=pos[dcIt->index1]-pos[dcIt->index0];
			Scalar d2=d.sqr();
			Scalar dScale;
			if(d2>=Scalar(1.0e-8))
				{
				/* Calculate the scale factor to bring the distance vector to the desired length: */
				dScale=(Scalar(1)-dcIt->dist/Math::sqrt(d2))*dcIt->strength*distConstraintScale;
				// dScale=(Scalar(1)-Scalar(2)*dcIt->dist2/(d2+dcIt->dist2))*dcIt->strength*distConstraintScale;
				}
			else
				{
				/* Create some "random" displacement vector to move the particles apart: */
				d=Vector(1,0,0);
				dScale=dcIt->dist*dcIt->strength*distConstraintScale;
				}
			
			/* Scale the update vector by the maximum number of distance constraints on both particles: */
			dScale/=Scalar(Math::max(numDistConstraints[dcIt->index0],numDistConstraints[dcIt->index1]));
			
			/* Check if at least one of the particles has finite mass: */
			if(imSum>Scalar(0))
				{
				/* Distribute the distance correction vector between the particles based on their masses: */
				pds[dcIt->index0]+=d*(dScale*im0/imSum);
				pds[dcIt->index1]-=d*(dScale*im1/imSum);
				}
			else
				{
				/* Distribute the distance correction vector between the particles evenly: */
				d*=Math::div2(dScale);
				pds[dcIt->index0]+=d;
				pds[dcIt->index1]-=d;
				}
			}
		
		/* Synchronize between all threads to switch from per-constraint back to per-particle parallelization: */
		if(barrier!=0)
			barrier->synchronize();
		
		/* Update this thread's particle positions: */
		for(unsigned int ti=0;ti<numThreads;++ti)
			{
			Vector* pdPtr=particleDeltas+(size_t(ti)*size_t(numParticles)+size_t(iBegin));
			for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt,++pdPtr)
				*pIt+=*pdPtr;
			}
		
		#if 0 // This can't go here -- the octree isn't up-to-date!
		if(minParticleDist2>Scalar(0))
			{
			/* Enforce a minimum distance between any pairs of particles: */
			for(Index index0=0;index0<numParticles-1;++index0)
				{
				EnforceMinDistFunctor emdf(*this,index0,minParticleDist2);
				octree.processCloseParticles(emdf);
				}
			}
		#endif
		
		/* Process all box constraints: */
		for(std::vector<BoxConstraint>::iterator bcIt=boxConstraints.begin();bcIt!=boxConstraints.end();++bcIt)
			{
			/* Check the type of box constraint: */
			if(bcIt->inside)
				{
				/* Keep all particles inside of the box: */
				for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt)
					for(int i=0;i<3;++i)
						{
						if((*pIt)[i]<bcIt->min[i])
							(*pIt)[i]=bcIt->min[i];
						else if((*pIt)[i]>bcIt->max[i])
							(*pIt)[i]=bcIt->max[i];
						}
				}
			else
				{
				/* Keep all particles outside of the box: */
				for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt)
					{
					bool inside=true;
					Scalar minDepth=Math::Constants<Scalar>::max;
					int minAxis=0;
					for(int i=0;i<3&&inside;++i)
						{
						inside=(*pIt)[i]>bcIt->min[i]&&(*pIt)[i]<bcIt->max[i];
						if(inside)
							{
							Scalar mid=Math::mid(bcIt->min[i],bcIt->max[i]);
							if((*pIt)[i]<mid)
								{
								Scalar depth=(*pIt)[i]-bcIt->min[i];
								if(minDepth>depth)
									{
									minDepth=depth;
									minAxis=-i-1;
									}
								}
							else
								{
								Scalar depth=bcIt->max[i]-(*pIt)[i];
								if(minDepth>depth)
									{
									minDepth=depth;
									minAxis=i+1;
									}
								}
							}
						}
					
					if(inside)
						{
						if(minAxis<0)
							(*pIt)[-minAxis-1]-=minDepth;
						else
							(*pIt)[minAxis-1]+=minDepth;
						}
					}
				}
			}
	
		/* Process all sphere constraints: */
		for(std::vector<SphereConstraint>::iterator scIt=sphereConstraints.begin();scIt!=sphereConstraints.end();++scIt)
			{
			/* Check the type of sphere constraint: */
			if(scIt->inside)
				{
				/* Keep all particles inside of the sphere: */
				for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt)
					{
					/* Check if the particle is outside the sphere: */
					Scalar dist2=Geometry::sqrDist(*pIt,scIt->center);
					if(dist2>scIt->radius2)
						{
						/* Project the particle back to the surface of the sphere: */
						*pIt+=(*pIt-scIt->center)*(scIt->radius/Math::sqrt(dist2)-Scalar(1));
						}
					}
				}
			else
				{
				/* Keep all particles outside of the sphere: */
				for(std::vector<Point>::iterator pIt=pBegin;pIt!=pEnd;++pIt)
					{
					/* Check if the particle is inside the sphere: */
					Scalar dist2=Geometry::sqrDist(*pIt,scIt->center);
					if(dist2<scIt->radius2)
						{
						/* Project the particle back to the surface of the sphere: */
						*pIt+=(*pIt-scIt->center)*(scIt->radius/Math::sqrt(dist2)-Scalar(1));
						}
					}
				}
			}
		}
	
	/* Synchronize between all threads and let one thread update the particle octree, then synchronize again: */
	bool updateOctree=true;
	if(barrier!=0)
		updateOctree=barrier->synchronize();
	if(updateOctree)
		{
		/* Update the particle octree: */
		octree.updateParticles();
		}
	if(barrier!=0)
		barrier->synchronize();
	}

namespace {

/****************
Helper functions:
****************/

void drawBox(bool inside,const Point& min,const Point& max,Scalar epsilon)
	{
	if(inside)
		{
		glBegin(GL_QUADS);
		glNormal3d( 1.0, 0.0, 0.0);
		glVertex(min[0]-epsilon,min[1]-epsilon,min[2]-epsilon);
		glVertex(min[0]-epsilon,max[1]+epsilon,min[2]-epsilon);
		glVertex(min[0]-epsilon,max[1]+epsilon,max[2]+epsilon);
		glVertex(min[0]-epsilon,min[1]-epsilon,max[2]+epsilon);
		
		glNormal(-1.0, 0.0, 0.0);
		glVertex(max[0]+epsilon,max[1]+epsilon,min[2]-epsilon);
		glVertex(max[0]+epsilon,min[1]-epsilon,min[2]-epsilon);
		glVertex(max[0]+epsilon,min[1]-epsilon,max[2]+epsilon);
		glVertex(max[0]+epsilon,max[1]+epsilon,max[2]+epsilon);
		
		glNormal( 0.0, 1.0, 0.0);
		glVertex(max[0]+epsilon,min[1]-epsilon,min[2]-epsilon);
		glVertex(min[0]-epsilon,min[1]-epsilon,min[2]-epsilon);
		glVertex(min[0]-epsilon,min[1]-epsilon,max[2]+epsilon);
		glVertex(max[0]+epsilon,min[1]-epsilon,max[2]+epsilon);
		
		glNormal( 0.0,-1.0, 0.0);
		glVertex(min[0]-epsilon,max[1]+epsilon,min[2]-epsilon);
		glVertex(max[0]+epsilon,max[1]+epsilon,min[2]-epsilon);
		glVertex(max[0]+epsilon,max[1]+epsilon,max[2]+epsilon);
		glVertex(min[0]-epsilon,max[1]+epsilon,max[2]+epsilon);
		
		glNormal( 0.0, 0.0, 1.0);
		glVertex(min[0]-epsilon,min[1]-epsilon,min[2]-epsilon);
		glVertex(max[0]+epsilon,min[1]-epsilon,min[2]-epsilon);
		glVertex(max[0]+epsilon,max[1]+epsilon,min[2]-epsilon);
		glVertex(min[0]-epsilon,max[1]+epsilon,min[2]-epsilon);
		
		glNormal( 0.0, 0.0,-1.0);
		glVertex(max[0]+epsilon,min[1]-epsilon,max[2]+epsilon);
		glVertex(min[0]-epsilon,min[1]-epsilon,max[2]+epsilon);
		glVertex(min[0]-epsilon,max[1]+epsilon,max[2]+epsilon);
		glVertex(max[0]+epsilon,max[1]+epsilon,max[2]+epsilon);
		glEnd();
		}
	else
		{
		glBegin(GL_QUADS);
		glNormal( 1.0, 0.0, 0.0);
		glVertex(max[0]-epsilon,min[1]+epsilon,min[2]+epsilon);
		glVertex(max[0]-epsilon,max[1]-epsilon,min[2]+epsilon);
		glVertex(max[0]-epsilon,max[1]-epsilon,max[2]-epsilon);
		glVertex(max[0]-epsilon,min[1]+epsilon,max[2]-epsilon);
		
		glNormal(-1.0, 0.0, 0.0);
		glVertex(min[0]+epsilon,max[1]-epsilon,min[2]+epsilon);
		glVertex(min[0]+epsilon,min[1]+epsilon,min[2]+epsilon);
		glVertex(min[0]+epsilon,min[1]+epsilon,max[2]-epsilon);
		glVertex(min[0]+epsilon,max[1]-epsilon,max[2]-epsilon);
		
		glNormal( 0.0, 1.0, 0.0);
		glVertex(max[0]-epsilon,max[1]-epsilon,min[2]+epsilon);
		glVertex(min[0]+epsilon,max[1]-epsilon,min[2]+epsilon);
		glVertex(min[0]+epsilon,max[1]-epsilon,max[2]-epsilon);
		glVertex(max[0]-epsilon,max[1]-epsilon,max[2]-epsilon);
		
		glNormal( 0.0,-1.0, 0.0);
		glVertex(min[0]+epsilon,min[1]+epsilon,min[2]+epsilon);
		glVertex(max[0]-epsilon,min[1]+epsilon,min[2]+epsilon);
		glVertex(max[0]-epsilon,min[1]+epsilon,max[2]-epsilon);
		glVertex(min[0]+epsilon,min[1]+epsilon,max[2]-epsilon);
		
		glNormal( 0.0, 0.0, 1.0);
		glVertex(min[0]+epsilon,min[1]+epsilon,max[2]-epsilon);
		glVertex(max[0]-epsilon,min[1]+epsilon,max[2]-epsilon);
		glVertex(max[0]-epsilon,max[1]-epsilon,max[2]-epsilon);
		glVertex(min[0]+epsilon,max[1]-epsilon,max[2]-epsilon);
		
		glNormal( 0.0, 0.0,-1.0);
		glVertex(max[0]-epsilon,min[1]+epsilon,min[2]+epsilon);
		glVertex(min[0]+epsilon,min[1]+epsilon,min[2]+epsilon);
		glVertex(min[0]+epsilon,max[1]-epsilon,min[2]+epsilon);
		glVertex(max[0]-epsilon,max[1]-epsilon,min[2]+epsilon);
		glEnd();
		}
	}

}

void ParticleSystem::glRenderAction(bool transparent) const
	{
	if(transparent)
		{
		#if 0
		/* Render the octree: */
		octree.glRenderAction();
		#endif
		}
	else
		{
		Scalar epsilon(0.01);
		
		/* Render all box constraints: */
		for(std::vector<BoxConstraint>::const_iterator bcIt=boxConstraints.begin();bcIt!=boxConstraints.end();++bcIt)
			drawBox(bcIt->inside,bcIt->min,bcIt->max,epsilon);
		
		/* Render all sphere constraints: */
		for(std::vector<SphereConstraint>::const_iterator scIt=sphereConstraints.begin();scIt!=sphereConstraints.end();++scIt)
			{
			glPushMatrix();
			glTranslated(scIt->center[0],scIt->center[1],scIt->center[2]);
			glFrontFace(scIt->inside?GL_CW:GL_CCW);
			glDrawSphereIcosahedron(scIt->radius-epsilon,6);
			glPopMatrix();
			}
		glFrontFace(GL_CCW);
		}
	}
