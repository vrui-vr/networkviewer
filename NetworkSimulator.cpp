/***********************************************************************
NetworkSimulator - Class to encapsulate a network layout simulator
running in its own thread.
Copyright (c) 2020-2023 Oliver Kreylos

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

#include "NetworkSimulator.h"

#include <Threads/FunctionCalls.h>
#include <Math/Math.h>

#include "Network.h"
#include "ParticleOctree.icpp"
#include "ForceFunctors.h"

#define BENCHMARK_SIMULATION 0

#if BENCHMARK_SIMULATION
#include <iostream>
#include <Realtime/Time.h>
#endif

/****************************************************
Methods of class NetworkSimulator::SimulationCommand:
****************************************************/

NetworkSimulator::SimulationCommand::~SimulationCommand(void)
	{
	}

/****************************************************
Methods of class NetworkSimulator::SelectNodeCommand:
****************************************************/

void NetworkSimulator::SelectNodeCommand::execute(NetworkSimulator& simulator)
	{
	Network& network=simulator.network;
	
	/* Change the node's selection state: */
	switch(mode)
		{
		case 0: // Select node
			network.selectNode(nodeIndex);
			
			break;
		
		case 1: // Deselect node
			network.deselectNode(nodeIndex);
			
			break;
		
		case 2: // Toggle node's selection state
			if(network.isSelected(nodeIndex))
				network.deselectNode(nodeIndex);
			else
				network.selectNode(nodeIndex);
			
			break;
		}
	}

/*********************************************************
Methods of class NetworkSimulator::ChangeSelectionCommand:
*********************************************************/

void NetworkSimulator::ChangeSelectionCommand::execute(NetworkSimulator& simulator)
	{
	Network& network=simulator.network;
	
	/* Change the network's selection set: */
	switch(command)
		{
		case 0: // Clear selection
			network.clearSelection();
			
			break;
		
		case 1: // Grow selection
			network.growSelection();
			
			break;
		
		case 2: // Shrink selection
			network.shrinkSelection();
			
			break;
		}
	}

/***************************************************
Methods of class NetworkSimulator::DragStartCommand:
***************************************************/

void NetworkSimulator::DragStartCommand::dragParticle(ParticleSystem& particles,NetworkSimulator::ActiveDrag& ad,bool* nodeDrags,Index nodeIndex)
	{
	/* Ensure that the particle is not part of another active drag: */
	if(!nodeDrags[nodeIndex])
		{
		/* Create a drag state for the requested particle: */
		ActiveDrag::DraggedParticle dp;
		dp.index=nodeIndex;
		dp.dragPos=initialTransform.inverseTransform(particles.getParticlePosition(nodeIndex));
		
		/* Set the requested particle's mass to zero: */
		dp.savedInvMass=particles.getParticleInvMass(nodeIndex);
		particles.setParticleInvMass(nodeIndex,Scalar(0));
		
		/* Add the requested particle to the drag: */
		ad.draggedParticles.push_back(dp);
		nodeDrags[nodeIndex]=true;
		}
	}

void NetworkSimulator::DragStartCommand::execute(NetworkSimulator& simulator)
	{
	Network& network=simulator.network;
	ParticleSystem& particles=simulator.particles;
	ActiveDragSet& activeDrags=simulator.activeDrags;
	bool* nodeDrags=simulator.nodeDrags;
	
	/* Create a new active drag state: */
	ActiveDrag& ad=activeDrags[ClientDragID(clientId,dragId)].getDest();
	ad.dragTransform=initialTransform;
	
	/* Check if the requested particle is part of the selection: */
	if(network.isSelected(pickedNodeIndex))
		{
		/* Drag the entire selection: */
		const Network::Selection& selection=network.getSelection();
		for(Network::Selection::ConstIterator sIt=selection.begin();!sIt.isFinished();++sIt)
			dragParticle(particles,ad,nodeDrags,sIt->getSource());
		}
	else
		{
		/* Only drag the picked particle: */
		dragParticle(particles,ad,nodeDrags,pickedNodeIndex);
		}
	
	/* Check if any particles were selected for dragging: */
	if(ad.draggedParticles.empty())
		{
		/* Remove the active drag state from the set again: */
		activeDrags.removeEntry(ClientDragID(clientId,dragId));
		}
	}

/**********************************************
Methods of class NetworkSimulator::DragCommand:
**********************************************/

void NetworkSimulator::DragCommand::execute(NetworkSimulator& simulator)
	{
	ActiveDragSet& activeDrags=simulator.activeDrags;
	
	/* Access the active drag state: */
	ActiveDragSet::Iterator adIt=activeDrags.findEntry(ClientDragID(clientId,dragId));
	if(!adIt.isFinished())
		{
		/* Update the drag state's drag transform: */
		adIt->getDest().dragTransform=dragTransform;;
		}
	}

/**************************************************
Methods of class NetworkSimulator::DragStopCommand:
**************************************************/

void NetworkSimulator::DragStopCommand::execute(NetworkSimulator& simulator)
	{
	ParticleSystem& particles=simulator.particles;
	ActiveDragSet& activeDrags=simulator.activeDrags;
	bool* nodeDrags=simulator.nodeDrags;
	
	/* Access the active drag state: */
	ActiveDragSet::Iterator adIt=activeDrags.findEntry(ClientDragID(clientId,dragId));
	if(!adIt.isFinished())
		{
		ActiveDrag& ad=adIt->getDest();
		
		/* Remove all dragged particles from the drag: */
		for(ActiveDrag::DraggedParticleList::iterator dpIt=ad.draggedParticles.begin();dpIt!=ad.draggedParticles.end();++dpIt)
			{
			/* Restore the particle's original inverse mass: */
			particles.setParticleInvMass(dpIt->index,dpIt->savedInvMass);
			
			/* Remove the particle from the drag: */
			nodeDrags[dpIt->index]=false;
			}
		
		/* Remove the active drag state from the set: */
		activeDrags.removeEntry(adIt);
		}
	}

/*********************************
Methods of class NetworkSimulator:
*********************************/

namespace {

/****************
Helper functions:
****************/

template <class ForceFunctorParam>
inline
void
applyForceFunctor(
	ParticleSystem& particles,
	const SimulationParameters& simulationParameters,
	Scalar forceFactor,
	Index iBegin,
	Index iEnd)
	{
	/* Access the particle system's octree: */
	const ParticleOctree& octree=particles.getOctree();
	
	/* Create a global force functor: */
	ForceFunctorParam gff(simulationParameters.repellingForceTheta,simulationParameters.repellingForceCutoff);
	for(Index index=iBegin;index<iEnd;++index)
		{
		gff.prepareParticle(index,particles.getParticlePosition(index));
		octree.calcForce(gff);
		particles.forceParticle(index,gff.getForce(),forceFactor);
		}
	}

}

void NetworkSimulator::innerUpdateLoopIteration(Scalar dt,unsigned int threadIndex)
	{
	#if BENCHMARK_SIMULATION
	Realtime::TimePointMonotonic timer;
	#endif
	
	/* Access the current simulation parameters: */
	const SimulationParameters& sp=simulationParameters.getLockedValue();
	Scalar dt2=Math::sqr(dt);
	
	/* Advance the particle system's state: */
	particles.moveParticles(dt,threadIndex);
	
	#if BENCHMARK_SIMULATION
	double time1=double(timer.setAndDiff())*1000.0;
	#endif
	
	/* Calculate the range of particles that will be updated by this thread: */
	Index iBegin=(threadIndex*particles.getNumParticles())/particles.getNumThreads();
	Index iEnd=((threadIndex+1)*particles.getNumParticles())/particles.getNumThreads();
	
	/* Pull all particles towards the network's center of gravity: */
	const ParticleOctree& octree=particles.getOctree();
	Point center=Point::origin; // octree.getCenterOfGravity(); // Pull towards the coordinate system's origin for now
	Scalar cff=sp.centralForce*dt2;
	for(Index index=iBegin;index<iEnd;++index)
		{
		const Point& pos=particles.getParticlePosition(index);
		Vector d=center-pos;
		particles.forceParticle(index,d,cff);
		}
	
	#if BENCHMARK_SIMULATION
	double time2=double(timer.setAndDiff())*1000.0;
	#endif
	
	/* Apply a repelling n-body force to all particles: */
	switch(sp.repellingForceMode)
		{
		case SimulationParameters::Linear:
			{
			/* Apply an inverse linear law force function: */
			applyForceFunctor<GlobalRepulsiveForceFunctorLinear>(particles,sp,sp.repellingForce*dt2,iBegin,iEnd);
			
			break;
			}
		
		case SimulationParameters::Quadratic:
			{
			/* Apply an inverse square law force function: */
			applyForceFunctor<GlobalRepulsiveForceFunctorQuadratic>(particles,sp,sp.repellingForce*dt2,iBegin,iEnd);
			
			break;
			}
		}
	
	#if BENCHMARK_SIMULATION
	double time3=double(timer.setAndDiff())*1000.0;
	#endif
	
	/* Finish the particle system's update step: */
	particles.enforceConstraints(dt,threadIndex);
	
	#if BENCHMARK_SIMULATION
	double time4=double(timer.setAndDiff())*1000.0;
	if(threadIndex==0)
		std::cout<<"Simulation times: "<<time1<<", "<<time2<<", "<<time3<<", "<<time4<<std::endl;
	#endif
	}

void* NetworkSimulator::simulationWorkerThreadMethod(unsigned int threadIndex)
	{
	/* Run the inner simulation loop until told to stop: */
	while(keepSimulationThreadRunning)
		{
		/* Synchronize with the other worker threads: */
		barrier.synchronize();
		
		/* Run one iteration of the inner update loop: */
		Scalar dt(1.0/60.0); // Use a fixed time step for now
		innerUpdateLoopIteration(dt,threadIndex);
		}
	
	return 0;
	}

void* NetworkSimulator::simulationThreadMethod(void)
	{
	/* Start a timer to enforce a maximum update rate: */
	Realtime::TimePointMonotonic nextUpdateTime;
	
	/* Run the simulation until told to stop: */
	while(keepSimulationThreadRunning)
		{
		/* Check if the simulation thread should be paused: */
		{
		Threads::MutexCond::Lock pauseSimulationThreadLock(pauseSimulationThreadCond);
		while(pauseSimulationThread)
			pauseSimulationThreadCond.wait(pauseSimulationThreadLock);
		}
		
		/* Lock the most recent simulation parameters: */
		if(simulationParameters.lockNewValue())
			{
			/* Update the particle system's state: */
			const SimulationParameters& sp=simulationParameters.getLockedValue();
			if(particles.getAttenuation()!=sp.attenuation)
				particles.setAttenuation(sp.attenuation);
			if(particles.getDistConstraintScale()!=sp.linkStrength)
				particles.setDistConstraintScale(sp.linkStrength);
			}
		
		/* Execute all queued simulation commands: */
		{
		SimulationCommandList scs;
		{
		Threads::Spinlock::Lock simulationCommandsLock(simulationCommandsMutex);
		std::swap(simulationCommands,scs);
		}
		for(SimulationCommandList::iterator scIt=scs.begin();scIt!=scs.end();++scIt)
			{
			/* Execute the command object and delete it: */
			(*scIt)->execute(*this);
			delete *scIt;
			}
		}
		
		/* Apply all active drag operations: */
		for(ActiveDragSet::Iterator adIt=activeDrags.begin();!adIt.isFinished();++adIt)
			{
			ActiveDrag& ad=adIt->getDest();
			for(ActiveDrag::DraggedParticleList::iterator dpIt=ad.draggedParticles.begin();dpIt!=ad.draggedParticles.end();++dpIt)
				particles.setParticlePosition(dpIt->index,ad.dragTransform.transform(dpIt->dragPos));
			}
		
		/* Synchronize with the worker threads: */
		if(numWorkerThreads>0)
			barrier.synchronize();
		
		/* Run one iteration of the inner update loop: */
		Scalar dt(1.0/60.0); // Use a fixed time step for now
		innerUpdateLoopIteration(dt,0);
		
		/* Check if it's time to send a simulation update: */
		Realtime::TimePointMonotonic currentTime;
		if(currentTime>=nextUpdateTime)
			{
			/* Call the simulation update callback: */
			(*simulationUpdateCallback)(particles);
			
			/* Advance the update timer: */
			nextUpdateTime+=Realtime::TimeVector(updateInterval);
			
			/* Check if we missed a time step: */
			if(currentTime>=nextUpdateTime)
				{
				/* Restart the update timer: */
				nextUpdateTime=currentTime;
				}
			}
		}
	
	return 0;
	}

void NetworkSimulator::queueCommand(NetworkSimulator::SimulationCommand* command)
	{
	/* Lock the simulation command queue and add the given command: */
	Threads::Spinlock::Lock simulationCommandsLock(simulationCommandsMutex);
	simulationCommands.push_back(command);
	}

NetworkSimulator::NetworkSimulator(Network& sNetwork,const SimulationParameters& sSimulationParameters,SimulationUpdateCallback& sSimulationUpdateCallback,unsigned int sNumWorkerThreads)
	:network(sNetwork),
	 keepSimulationThreadRunning(true),pauseSimulationThread(false),
	 numWorkerThreads(sNumWorkerThreads),workerThreads(0),
	 activeDrags(17),nodeDrags(new bool[network.getNodes().size()]),
	 updateInterval(1.0/30.0),simulationUpdateCallback(&sSimulationUpdateCallback)
	{
	/* Initialize the particle system: */
	particles.setGravity(Vector::zero);
	particles.setAttenuation(sSimulationParameters.attenuation);
	particles.setDistConstraintScale(sSimulationParameters.linkStrength);
	particles.setNumRelaxationIterations(sSimulationParameters.numRelaxationIterations);
	
	/* Create particles for all network nodes and distance constraints for all network links: */
	network.createParticles(particles,Scalar(1));
	particles.finishUpdate();
	
	/* Initialize the node drag flags: */
	for(size_t i=0;i<network.getNodes().size();++i)
		nodeDrags[i]=false;
	
	/* Post initial simulation parameters: */
	simulationParameters.postNewValue(sSimulationParameters);
	
	/* Start the optional addition worker threads: */
	if(numWorkerThreads>0)
		{
		/* Set the number of threads synchronizing on the barrier: */
		barrier.setNumSynchronizingThreads(1+numWorkerThreads);
		
		/* Enable parallelism in the particle system: */
		particles.setNumThreads(1+numWorkerThreads,barrier);
		
		workerThreads=new Threads::Thread[numWorkerThreads];
		
		for(unsigned int ti=0;ti<numWorkerThreads;++ti)
			workerThreads[ti].start(this,&NetworkSimulator::simulationWorkerThreadMethod,ti+1);
		}
	
	/* Start the main simulation thread: */
	simulationThread.start(this,&NetworkSimulator::simulationThreadMethod);
	}

NetworkSimulator::~NetworkSimulator(void)
	{
	/* Tell the simulation thread to stop: */
	keepSimulationThreadRunning=false;
	
	/* Wake up the simulation thread in case it was paused: */
	{
	Threads::MutexCond::Lock pauseSimulationThreadLock(pauseSimulationThreadCond);
	pauseSimulationThread=false;
	pauseSimulationThreadCond.signal();
	}
	
	/* Wait for the simulation thread to terminate: */
	simulationThread.join();
	
	/* Wait for the optional additional worker threads to terminate: */
	if(numWorkerThreads>0)
		{
		for(unsigned int ti=0;ti<numWorkerThreads;++ti)
			workerThreads[ti].join();
		
		delete[] workerThreads;
		}
	
	/* Delete all pending simulation commands: */
	for(SimulationCommandList::iterator scIt=simulationCommands.begin();scIt!=simulationCommands.end();++scIt)
		delete *scIt;
	
	/* Clean up: */
	delete[] nodeDrags;
	}

void NetworkSimulator::setUpdateInterval(double newUpdateInterval)
	{
	/* Set the update interval; background thread will grab it when needed: */
	updateInterval=newUpdateInterval;
	}

void NetworkSimulator::pause(void)
	{
	/* Tell the simulation thread to pause: */
	pauseSimulationThread=true;
	}

void NetworkSimulator::resume(void)
	{
	/* Reset the pause flag and wake up the simulation thread: */
	Threads::MutexCond::Lock pauseSimulationThreadLock(pauseSimulationThreadCond);
	pauseSimulationThread=false;
	pauseSimulationThreadCond.signal();
	}
