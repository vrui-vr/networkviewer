/***********************************************************************
NetworkSimulator - Class to encapsulate a network layout simulator
running in its own thread.
Copyright (c) 2023 Oliver Kreylos

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

#ifndef NETWORKSIMULATOR_INCLUDED
#define NETWORKSIMULATOR_INCLUDED

#include <vector>
#include <Misc/OrderedTuple.h>
#include <Misc/Autopointer.h>
#include <Misc/HashTable.h>
#include <Realtime/Time.h>
#include <Threads/Spinlock.h>
#include <Threads/MutexCond.h>
#include <Threads/Thread.h>
#include <Threads/Barrier.h>
#include <Threads/TripleBuffer.h>
#include <Geometry/OrthonormalTransformation.h>

#include "ParticleTypes.h"
#include "ParticleSystem.h"
#include "SimulationParameters.h"

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
}
class Network;

class NetworkSimulator
	{
	/* Embedded classes: */
	public:
	typedef Threads::FunctionCall<const ParticleSystem&> SimulationUpdateCallback; // Type for functions called from simulation thread when a simulation update is available
	
	typedef Geometry::OrthonormalTransformation<Scalar,3> DragTransform; // Type for dragging transformations
	
	private:
	struct ActiveDrag // Structure representing an active dragging operation
		{
		/* Embedded classes: */
		public:
		struct DraggedParticle // State of one dragged particle
			{
			/* Elements: */
			public:
			Index index; // Particle's index
			Scalar savedInvMass; // Particle's original inverse mass
			Point dragPos; // Particle's position relative to drag transformation
			};
		
		typedef std::vector<DraggedParticle> DraggedParticleList;
		
		/* Elements: */
		DraggedParticleList draggedParticles; // List of particles affected by the dragging operation
		DragTransform dragTransform; // Current drag transformation
		};
	
	typedef Misc::OrderedTuple<unsigned int,2> ClientDragID; // Server-wide unique drag operation ID consisting of a client ID and a drag ID
	typedef Misc::HashTable<ClientDragID,ActiveDrag> ActiveDragSet; // Map from drag operation IDs to active dragging operation states
	
	class SimulationCommand // Base class for commands issued to the simulation thread from the main thread
		{
		/* Constructors and destructors: */
		public:
		virtual ~SimulationCommand(void);
		
		/* Methods: */
		virtual void execute(NetworkSimulator& simulator) =0; // Executes the command
		};
	
	class SelectNodeCommand:public SimulationCommand // Class to select/deselect nodes
		{
		/* Elements: */
		private:
		Index nodeIndex; // Index of the node whose selection state is to be changed
		int mode; // Change mode: 0: select node; 1: deselect node; 2: toggle node's selection state
		
		/* Constructors and destructors: */
		public:
		SelectNodeCommand(Index sNodeIndex,int sMode)
			:nodeIndex(sNodeIndex),mode(sMode)
			{
			}
		
		/* Methods from class SimulationCommand: */
		virtual void execute(NetworkSimulator& simulator);
		};
	
	class ChangeSelectionCommand:public SimulationCommand // Class to make global changes to the selection set
		{
		/* Elements: */
		private:
		int command; // Change command: 0: clear selection, 1: grow selection, 2: shrink selection
		
		/* Constructors and destructors: */
		public:
		ChangeSelectionCommand(int sCommand)
			:command(sCommand)
			{
			}
		
		/* Methods from class SimulationCommand: */
		virtual void execute(NetworkSimulator& simulator);
		};
	
	class DragStartCommand:public SimulationCommand // Class to start dragging operations
		{
		/* Elements: */
		private:
		unsigned int clientId;
		unsigned int dragId;
		Index pickedNodeIndex;
		DragTransform initialTransform;
		
		/* Private methods: */
		void dragParticle(ParticleSystem& particles,ActiveDrag& ad,bool* nodeDrags,Index nodeIndex); // Adds the requested particle to the active drag set if it is not already being dragged
		
		/* Constructors and destructors: */
		public:
		DragStartCommand(unsigned int sClientId,unsigned int sDragId,Index sPickedNodeIndex,const DragTransform& sInitialTransform)
			:clientId(sClientId),dragId(sDragId),pickedNodeIndex(sPickedNodeIndex),initialTransform(sInitialTransform)
			{
			}
		
		/* Methods from class SimulationCommand: */
		virtual void execute(NetworkSimulator& simulator);
		};
	
	class DragCommand:public SimulationCommand // Class to continue dragging operations
		{
		/* Elements: */
		private:
		unsigned int clientId;
		unsigned int dragId;
		DragTransform dragTransform;
		
		/* Constructors and destructors: */
		public:
		DragCommand(unsigned int sClientId,unsigned int sDragId,const DragTransform& sDragTransform)
			:clientId(sClientId),dragId(sDragId),dragTransform(sDragTransform)
			{
			}
		
		/* Methods from class SimulationCommand: */
		virtual void execute(NetworkSimulator& simulator);
		};
	
	class DragStopCommand:public SimulationCommand // Class to stop dragging operations
		{
		/* Elements: */
		private:
		unsigned int clientId;
		unsigned int dragId;
		
		/* Constructors and destructors: */
		public:
		DragStopCommand(unsigned int sClientId,unsigned int sDragId)
			:clientId(sClientId),dragId(sDragId)
			{
			}
		
		/* Methods from class SimulationCommand: */
		virtual void execute(NetworkSimulator& simulator);
		};
	
	friend class SelectNodeCommand;
	friend class DragStartCommand;
	friend class DragCommand;
	friend class DragStopCommand;
	
	typedef std::vector<SimulationCommand*> SimulationCommandList; // Type for lists of simulation commands sent from the main thread to the simulation thread
	
	/* Elements: */
	Network& network; // The visualized network
	ParticleSystem particles; // Particle system to simulate network node and link interactions
	Threads::TripleBuffer<SimulationParameters> simulationParameters; // Triple buffer of particle system simulation parameters
	volatile bool keepSimulationThreadRunning; // Flag to shut down the simulation thread
	volatile bool pauseSimulationThread; // Flag to pause the simulation thread while no clients are connected
	Threads::MutexCond pauseSimulationThreadCond; // Condition variable to wake up the simulation thread from being paused
	Threads::Thread simulationThread; // Background thread simulating the network
	unsigned int numWorkerThreads; // Number of additional worker threads cooperating with the background simulation thread
	Threads::Thread* workerThreads; // Array of additional threads cooperating with the background simulation thread
	Threads::Barrier barrier; // Barrier synchronizing between the background simulation thread and the worker threads
	ActiveDragSet activeDrags; // Map of active drag operations
	bool* nodeDrags; // Array of flags indicating whether each particle is being dragged
	volatile double updateInterval; // Time between network updates sent to clients
	Misc::Autopointer<SimulationUpdateCallback> simulationUpdateCallback; // Function called from simulation thread when a new update is available
	Threads::Spinlock simulationCommandsMutex; // Mutex serializing access to the simulation command list
	SimulationCommandList simulationCommands; // List holding commands from the front-end to the simulation thread
	
	/* Private methods: */
	void innerUpdateLoopIteration(Scalar dt,unsigned int threadIndex); // Runs one iteration of the network simulation update loop from a number of worker threads in parallel
	void* simulationWorkerThreadMethod(unsigned int threadIndex); // Method implementing a worker thread cooperating with the background simulation thread
	void* simulationThreadMethod(void); // Method implementing the background simulation thread
	void queueCommand(SimulationCommand* command); // Puts a new command into the simulation thread's queue
	
	/* Constructors and destructors: */
	public:
	NetworkSimulator(Network& sNetwork,const SimulationParameters& sSimulationParameters,SimulationUpdateCallback& sSimulationUpdateCallback,unsigned int sNumWorkerThreads =0); // Creates a simulator for the given network; takes ownership of callback object
	~NetworkSimulator(void);
	
	/* Methods: */
	void setSimulationParameters(const SimulationParameters& newSimulationParameters) // Sets the simulation parameters
		{
		/* Put the new set of parameters into the triple buffer: */
		simulationParameters.postNewValue(newSimulationParameters);
		}
	void setUpdateInterval(double newUpdateInterval); // Sets the time interval at which simulation updates are pushed to clients in seconds
	void pause(void); // Pauses the simulation thread
	void resume(void); // Resumes the simulation thread
	
	void selectNode(Index nodeIndex,int mode) // Changes a node's selection state
		{
		/* Queue a simulation command: */
		queueCommand(new SelectNodeCommand(nodeIndex,mode));
		}
	void changeSelection(int command) // Makes a global change to the selection set
		{
		/* Queue a simulation command: */
		queueCommand(new ChangeSelectionCommand(command));
		}
	void dragStart(unsigned int clientId,unsigned int dragId,Index pickedNodeIndex,const DragTransform& initialTransform) // Starts a dragging operation
		{
		/* Queue a new simulation command: */
		queueCommand(new DragStartCommand(clientId,dragId,pickedNodeIndex,initialTransform));
		}
	void drag(unsigned int clientId,unsigned int dragId,const DragTransform& dragTransform) // Continues a dragging operation
		{
		/* Queue a new simulation command: */
		queueCommand(new DragCommand(clientId,dragId,dragTransform));
		}
	void dragStop(unsigned int clientId,unsigned int dragId) // Finishes a dragging operation
		{
		/* Queue a new simulation command: */
		queueCommand(new DragStopCommand(clientId,dragId));
		}
	};

#endif
