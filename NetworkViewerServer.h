/***********************************************************************
NetworkViewerServer - Server for network viewer plug-in protocol.
Copyright (c) 2019-2023 Oliver Kreylos

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

#ifndef NETWORKVIEWERSERVER_INCLUDED
#define NETWORKVIEWERSERVER_INCLUDED

#include <string>
#include <vector>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Threads/Thread.h>
#include <Threads/Spinlock.h>
#include <Threads/MutexCond.h>
#include <Threads/TripleBuffer.h>
#include <Threads/EventDispatcher.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Collaboration2/PluginServer.h>
#include <Collaboration2/Plugins/MetadosisServer.h>
#include <Collaboration2/Plugins/VruiCoreProtocol.h>

#include "SimulationParameters.h"
#include "RenderingParameters.h"
#include "NetworkViewerProtocol.h"

/* Forward declarations: */
namespace Collab {
class MessageContinuation;
namespace Plugins {
class VruiCoreServer;
}
}
class Network;
class NetworkSimulator;
class ParticleSystem;

namespace Collab {

namespace Plugins {

class NetworkViewerServer:public PluginServer,public NetworkViewerProtocol
	{
	/* Embedded classes: */
	private:
	class Client:public PluginServer::Client // Class representing a client participating in the Network Viewer protocol
		{
		friend class NetworkViewerServer;
		
		/* Embedded classes: */
		private:
		struct ActiveDrag // Structure representing an active drag operation from this client
			{
			/* Elements: */
			public:
			unsigned int inputDeviceId; // ID of the input device associated with the dragging operation
			const VruiCoreProtocol::ClientInputDeviceState* deviceState; // Vrui Core state of the input device
			
			/* Constructors and destructors: */
			ActiveDrag(unsigned int sInputDeviceId,const VruiCoreProtocol::ClientInputDeviceState* sDeviceState)
				:inputDeviceId(sInputDeviceId),deviceState(sDeviceState)
				{
				}
			};
		
		typedef Misc::HashTable<unsigned int,ActiveDrag> ActiveDragMap; // Type mapping drag IDs to active drag states
		
		/* Elements: */
		private:
		unsigned int networkVersion; // Version number of the network that the client currently has
		ActiveDragMap activeDrags; // Map of active drag operations by drag ID
		
		/* Constructors and destructors: */
		Client(void);
		virtual ~Client(void);
		};
	
	typedef Misc::HashTable<unsigned int,void> NodeSet; // Type to represent sets of nodes
	
	struct ForwardNetworkCompleteCallbackData // Callback data structure when a network file has been completely forwarded to a client
		{
		/* Elements: */
		public:
		unsigned int clientId; // ID of the client who has received the network file
		Version networkVersion; // Version number of the network that was forwarded to the client
		
		/* Constructors and destructors: */
		ForwardNetworkCompleteCallbackData(unsigned int sClientId,Version sNetworkVersion)
			:clientId(sClientId),networkVersion(sNetworkVersion)
			{
			}
		};
	
	/* Elements: */
	MetadosisServer* metadosis; // Pointer to the Metadosis server plug-in
	VruiCoreServer* vruiCore; // Pointer to the Vrui Core server plug-in
	Threads::EventDispatcher::ListenerKey readNetworkJobCompleteSignalKey; // Event key to signal that a network file has been complete read in a background worker thread
	Threads::EventDispatcher::ListenerKey simulationUpdateSignalKey; // Event key to signal that a simulation update message is ready to broadcast
	Version networkVersion; // Version number of the current visualized network
	std::string networkName; // Name of the visualized network
	MetadosisServer::InStreamPtr networkFile; // The json file from which the visualized network was read
	Network* network; // The visualized network
	NodeSet labeledNodes; // Set of nodes currently displaying property labels
	SimulationParameters simulationParameters; // Most recent simulation parameters requested for the network simulator
	unsigned int numWorkerThreads; // Number of additional worker threads to use for network simulation
	NetworkSimulator* simulator; // Pointer to a simulator for the visualized network
	RenderingParameters renderingParameters; // Most recent rendering parameters
	
	/* Private methods: */
	void readNetworkJobCompleteCallback(Threads::EventDispatcher::SignalEvent& event); // Callback called when a network has been read from a network file in a background worker thread
	void forwardNetworkCompleteCallback(MetadosisProtocol::StreamID streamId,ForwardNetworkCompleteCallbackData cbData); // Callback called when a client has completely received a forwarded network file
	MessageContinuation* loadNetworkRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	MessageContinuation* setSimulationParametersRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	MessageContinuation* setRenderingParametersRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	MessageContinuation* selectNodeRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	MessageContinuation* changeSelectionRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	MessageContinuation* displayLabelRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	MessageContinuation* dragStartRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	MessageContinuation* dragRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	MessageContinuation* dragStopRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	void simulationUpdateCallback(const ParticleSystem& particles); // Callback called from the background simulation thread if a simulation update should be sent to clients
	void sendSimulationUpdateCallback(Threads::EventDispatcher::SignalEvent& event); // Callback called in the frontend when a simulation update should be sent to clients
	void loadNetworkCommandCallback(const char* argumentBegin,const char* argumentEnd);
	
	/* Constructors and destructors: */
	public:
	NetworkViewerServer(Server* sServer);
	virtual ~NetworkViewerServer(void);
	
	/* Methods from class PluginServer: */
	virtual const char* getName(void) const;
	virtual unsigned int getVersion(void) const;
	virtual unsigned int getNumClientMessages(void) const;
	virtual unsigned int getNumServerMessages(void) const;
	virtual void setMessageBases(unsigned int newClientMessageBase,unsigned int newServerMessageBase);
	virtual void start(void);
	virtual void clientConnected(unsigned int clientId);
	virtual void clientDisconnected(unsigned int clientId);
	};

}

}

#endif
