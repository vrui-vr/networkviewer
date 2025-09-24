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

#include "ParticleGrabber.h"

#include <Math/Math.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

class ParticleGrabberFactory:public Vrui::ToolFactory // Factory class for particle grabber tools
	{
	friend class ParticleGrabber;
	
	/* Elements: */
	private:
	ParticleSystem& particles; // Reference to the particle system from which particles are grabbed
	std::vector<Body*>* bodies; // Pointer to list of bodies that can be grabbed
	
	/* Constructors and destructors: */
	public:
	ParticleGrabberFactory(Vrui::ToolManager& toolManager,ParticleSystem& sParticles,std::vector<Body*>* sBodies);
	static void factoryDestructor(Vrui::ToolFactory* factory)
		{
		delete factory;
		}
	virtual ~ParticleGrabberFactory(void);
	
	/* Methods from class Vrui::ToolFactory: */
	virtual const char* getName(void) const;
	virtual Vrui::Tool* createTool(const Vrui::ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Vrui::Tool* tool) const;
	};

/***************************************
Methods of class ParticleGrabberFactory:
***************************************/

ParticleGrabberFactory::ParticleGrabberFactory(Vrui::ToolManager& toolManager,ParticleSystem& sParticles,std::vector<Body*>* sBodies)
	:Vrui::ToolFactory("ParticleGrabber",toolManager),
	 particles(sParticles),bodies(sBodies)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Set the custom tool class' factory pointer: */
	ParticleGrabber::factory=this;
	}

ParticleGrabberFactory::~ParticleGrabberFactory(void)
	{
	/* Reset the custom tool class' factory pointer: */
	ParticleGrabber::factory=0;
	}

const char* ParticleGrabberFactory::getName(void) const
	{
	return "Grab Particles";
	}

Vrui::Tool* ParticleGrabberFactory::createTool(const Vrui::ToolInputAssignment& inputAssignment) const
	{
	/* Return a new particle grabber: */
	return new ParticleGrabber(this,inputAssignment);
	}

void ParticleGrabberFactory::destroyTool(Vrui::Tool* tool) const
	{
	/* Destroy the tool: */
	delete tool;
	}

/****************************************
Static elements of class ParticleGrabber:
****************************************/

ParticleGrabberFactory* ParticleGrabber::factory=0;

/********************************
Methods of class ParticleGrabber:
********************************/

void ParticleGrabber::initClass(ParticleSystem& sParticles,std::vector<Body*>* sBodies)
	{
	/* Create a new particle grabber factory and register it with the tool manager: */
	Vrui::getToolManager()->addClass(new ParticleGrabberFactory(*Vrui::getToolManager(),sParticles,sBodies),ParticleGrabberFactory::factoryDestructor);
	}

ParticleGrabber::ParticleGrabber(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 particleGrabbed(false),grabbedBody(0)
	{
	}

ParticleGrabber::~ParticleGrabber(void)
	{
	if(grabbedBody!=0)
		{
		/* Release the grabbed body: */
		grabbedBody->grabRelease(grabId);
		}
	else if(particleGrabbed)
		{
		/* Reset the grabbed particle's inverse mass: */
		factory->particles.setParticleInvMass(particleIndex,particleInvMass);
		}
	}

const Vrui::ToolFactory* ParticleGrabber::getFactory(void) const
	{
	return factory;
	}

void ParticleGrabber::buttonCallback(int,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		Vrui::NavTransform devTrans=Vrui::getInverseNavigationTransformation()*Vrui::NavTransform(getButtonDeviceTransformation(0));
		Point pickPos(devTrans.getOrigin());
		
		if(factory->bodies!=0)
			{
			/* Try grabbing a body: */
			Body::GrabTransform grabTransform(Body::GrabTransform::Vector(devTrans.getTranslation()),Body::GrabTransform::Rotation(devTrans.getRotation()));
			Scalar pickRadius(Vrui::getPointPickDistance());
			for(std::vector<Body*>::iterator bIt=factory->bodies->begin();bIt!=factory->bodies->end();++bIt)
				{
				if((grabId=(*bIt)->grab(pickPos,pickRadius,grabTransform))!=Body::GrabID(0))
					{
					grabbedBody=*bIt;
					break;
					}
				}
			}
		
		if(grabbedBody==0)
			{
			/* Try grabbing a particle: */
			Index numParticles=factory->particles.getNumParticles();
			Index bestIndex=numParticles;
			Scalar bestDist2=Math::sqr(Scalar(Vrui::getPointPickDistance())*Scalar(5));
			for(Index index=0;index<numParticles;++index)
				{
				Scalar dist2=Geometry::sqrDist(pickPos,factory->particles.getParticlePosition(index));
				if(bestDist2>dist2)
					{
					bestIndex=index;
					bestDist2=dist2;
					}
				}
			
			if(bestIndex<numParticles)
				{
				/* Grab the picked particle: */
				particleGrabbed=true;
				particleIndex=bestIndex;
				particleInvMass=factory->particles.getParticleInvMass(particleIndex);
				factory->particles.setParticleInvMass(particleIndex,Scalar(0));
				particlePos=devTrans.inverseTransform(Vrui::Point(factory->particles.getParticlePosition(particleIndex)));
				}
			}
		}
	else if(grabbedBody!=0)
		{
		/* Release the grabbed body: */
		grabbedBody->grabRelease(grabId);
		grabbedBody=0;
		}
	else if(particleGrabbed)
		{
		/* Release the grabbed particle: */
		particleGrabbed=false;
		factory->particles.setParticleInvMass(particleIndex,particleInvMass);
		}
	}

void ParticleGrabber::frame(void)
	{
	/* Check if the tool is holding a body or a particle: */
	if(grabbedBody!=0)
		{
		/* Update the grabbed body with the new input device transformation: */
		Vrui::NavTransform devTrans=Vrui::getInverseNavigationTransformation()*Vrui::NavTransform(getButtonDeviceTransformation(0));
		Body::GrabTransform grabTransform(Body::GrabTransform::Vector(devTrans.getTranslation()),Body::GrabTransform::Rotation(devTrans.getRotation()));
		grabbedBody->grabUpdate(grabId,grabTransform);
		}
	else if(particleGrabbed)
		{
		/* Override the grabbed particle's position: */
		Vrui::NavTransform devTrans=Vrui::getInverseNavigationTransformation()*Vrui::NavTransform(getButtonDeviceTransformation(0));
		factory->particles.setParticlePosition(particleIndex,Point(devTrans.transform(particlePos)));
		}
	}
