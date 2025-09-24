/***********************************************************************
NetworkViewerClient - Client for network viewer plug-in protocol.
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

#ifndef NETWORKVIEWERCLIENT_INCLUDED
#define NETWORKVIEWERCLIENT_INCLUDED

#include <string>
#include <vector>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Threads/WorkerPool.h>
#include <IO/File.h>
#include <Collaboration2/PluginClient.h>
#include <Collaboration2/Plugins/MetadosisClient.h>

#include "NetworkViewerProtocol.h"

/* Forward declarations: */
namespace Collab {
class MessageReader;
class MessageContinuation;
class NonBlockSocket;
class Client;
}
class Network;
class SimulationParameters;
class RenderingParameters;
class CollaborativeNetworkViewer;

namespace Collab {

namespace Plugins {

class NetworkViewerClient:public PluginClient,public NetworkViewerProtocol
	{
	friend class ::CollaborativeNetworkViewer;
	
	/* Embedded classes: */
	public:
	typedef std::vector<NVPoint> NVPointList; // Type for lists of points
	
	private:
	class ReadNetworkJob:public Threads::WorkerPool::JobFunction // Class to read and parse a network file from a worker thread
		{
		/* Elements: */
		public:
		unsigned int networkVersion; // The version number of the network being read
		std::string networkName; // The name of the network being read
		IO::FilePtr networkFile; // The file from which to parse the network
		Network* network; // Pointer to the network object parsed from the network file
		Client* client; // Pointer to the collaboration client
		unsigned int serverMessageBase; // Base index for NetworkViewerProtocol server messages
		
		/* Constructors and destructors: */
		public:
		ReadNetworkJob(unsigned int sNetworkVersion,const std::string& sNetworkName,IO::File& sNetworkFile,Client* sClient,unsigned int sServerMessageBase); // Creates a job to load a network of the given name from the given Metadosis stream
		virtual ~ReadNetworkJob(void);
		
		/* Methods from class Threads::WorkerPool::JobFunction: */
		virtual void operator()(int) const;
		virtual void operator()(int);
		};
	
	typedef Misc::HashTable<unsigned int,void> ActiveDragSet; // Hash table holding the IDs of currently active drag operations
	
	/* Elements: */
	CollaborativeNetworkViewer* application; // Pointer to the collaborative network viewer application object
	MetadosisClient* metadosis; // Pointer to the Metadosis protocol's client plug-in
	Version networkVersion; // Version number of the visualized network
	std::string networkName; // The name of the visualized network
	Network* network; // The visualized network
	Version downloadingVersion; // Version number of the most recent network downloaded from the server
	std::vector<NodeID>* selectionSet; // Selection set to apply to the currently downloading network when it's done processing
	std::vector<NodeID>* labelSet; // Label set to apply to the currently downloading network when it's done processing
	DragID lastDragId; // Drag ID assigned to the most recent drag operation
	ActiveDragSet activeDrags; // Set of currently active drag IDs
	
	/* Private methods: */
	void loadNetworkCompleteNotificationCallback(unsigned int messageId,MessageReader& message);
	void loadNetworkNotificationCallback(unsigned int messageId,MessageReader& message);
	void selectionSetNotificationCallback(unsigned int messageId,MessageReader& message);
	void labelSetNotificationCallback(unsigned int messageId,MessageReader& message);
	void setSimulationParametersNotificationCallback(unsigned int messageId,MessageReader& message);
	void setRenderingParametersNotificationCallback(unsigned int messageId,MessageReader& message);
	void selectNodeNotificationCallback(unsigned int messageId,MessageReader& message);
	void changeSelectionNotificationCallback(unsigned int messageId,MessageReader& message);
	void displayLabelNotificationCallback(unsigned int messageId,MessageReader& message);
	MessageContinuation* simulationUpdateCallback(unsigned int messageId,MessageContinuation* continuation);
	
	/* Constructors and destructors: */
	public:
	NetworkViewerClient(CollaborativeNetworkViewer* sApplication,Client* sClient);
	virtual ~NetworkViewerClient(void);
	
	/* Methods from class PluginClient: */
	virtual const char* getName(void) const;
	virtual unsigned int getVersion(void) const;
	virtual unsigned int getNumClientMessages(void) const;
	virtual unsigned int getNumServerMessages(void) const;
	virtual void setMessageBases(unsigned int newClientMessageBase,unsigned int newServerMessageBase);
	virtual void start(void);
	
	/* New methods: */
	void loadNetwork(const char* networkFileName); // Loads a network from a file of the given name and shares it with the server
	const Network& getNetwork(void) const // Returns the visualized network
		{
		return *network;
		}
	void updateSimulationParameters(const SimulationParameters& simulationParameters);
	void selectNode(unsigned int pickedNodeIndex,unsigned int mode); // Selects/deselects/toggles the given node
	void changeSelection(unsigned int command); // Makes a global change to the selection set, 0: clear selection, 1: grow selection, 2: shrink selection
	void displayLabel(unsigned int nodeIndex,unsigned int command); // Sends a display label request to the server
	unsigned int startDrag(unsigned int inputDeviceId,unsigned int pickedNodeIndex); // Starts a new drag operation and returns its drag ID
	void drag(unsigned int dragId); // Continues the drag operation with the given drag ID
	void stopDrag(unsigned int dragId); // Stops the drag operation with the given drag ID
	void updateRenderingParameters(const RenderingParameters& renderingParameters);
	};

}

}

#endif
