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

#include "NetworkViewerServer.h"

#include <stdexcept>
#include <Misc/Utility.h>
#include <Misc/Autopointer.h>
#include <Misc/MessageLogger.h>
#include <Misc/CommandDispatcher.h>
#include <Threads/FunctionCalls.h>
#include <Threads/WorkerPool.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Collaboration2/MessageContinuation.h>
#include <Collaboration2/MessageWriter.h>
#include <Collaboration2/Server.h>
#include <Collaboration2/Plugins/VruiCoreServer.h>

#include "Network.h"
#include "ParticleSystem.h"
#include "NetworkSimulator.h"

namespace Collab {

namespace Plugins {

/********************************************
Methods of class NetworkViewerServer::Client:
********************************************/

NetworkViewerServer::Client::Client(void)
	:networkVersion(0),
	 activeDrags(5)
	{
	}

NetworkViewerServer::Client::~Client(void)
	{
	}

/************************************
Methods of class NetworkViewerServer:
************************************/

namespace {

/**************
Helper classes:
**************/

class ReadNetworkJob:public Threads::WorkerPool::JobFunction // Class to read and parse a network file from a worker thread
	{
	/* Elements: */
	public:
	std::string networkName; // The name of the network being read
	MetadosisProtocol::InStreamPtr networkFile; // The file from which to parse the network
	Network* network; // Pointer to the network object parsed from the network file
	const SimulationParameters& simulationParameters; // Reference to the current simulation parameters
	Misc::Autopointer<NetworkSimulator::SimulationUpdateCallback> simulationUpdateCallback; // Callback that will be called when the network simulator has a new state update
	unsigned int numWorkerThreads; // Number of additional worker threads to be used by the network simulator
	NetworkSimulator* simulator; // Pointer to a simulator for the parsed network
	
	/* Constructors and destructors: */
	public:
	ReadNetworkJob(const std::string& sNetworkName,MetadosisProtocol::InStream& sNetworkFile,const SimulationParameters& sSimulationParameters,NetworkSimulator::SimulationUpdateCallback& sSimulationUpdateCallback,unsigned int sNumWorkerThreads) // Creates a job to load a network of the given name from the given Metadosis stream
		:networkName(sNetworkName),networkFile(&sNetworkFile),
		 network(0),
		 simulationParameters(sSimulationParameters),simulationUpdateCallback(&sSimulationUpdateCallback),numWorkerThreads(sNumWorkerThreads),
		 simulator(0)
		{
		}
	virtual ~ReadNetworkJob(void)
		{
		/* Release allocated resources: */
		delete simulator;
		delete network;
		}
	
	/* Methods from class Threads::WorkerPool::JobFunction: */
	virtual void operator()(int) const
		{
		/* Can't really do anything; this is an API problem that should be fixed: */
		// ...
		}
	virtual void operator()(int)
		{
		try
			{
			/* Parse the network file into a network object: */
			network=new Network(*networkFile);
			
			/* Create a network simulator: */
			simulator=new NetworkSimulator(*network,simulationParameters,*simulationUpdateCallback,numWorkerThreads);
			}
		catch(const std::runtime_error& err)
			{
			/* Show an error message: */
			Misc::formattedUserError("NetworkViewer::ReadNetworkJob: Unable to read network %s due to exception %s",networkName.c_str(),err.what());
			}
		}
	};

}

void NetworkViewerServer::readNetworkJobCompleteCallback(Threads::EventDispatcher::SignalEvent& event)
	{
	/* Access the job object: */
	ReadNetworkJob* job=static_cast<ReadNetworkJob*>(event.getSignalData());
	
	/* Complete the read network request: */
	networkName=job->networkName;
	networkFile=job->networkFile;
	network=job->network;
	job->network=0;
	simulator=job->simulator;
	job->simulator=0;
	
	/* Release the job object: */
	job->unref();
	}

void NetworkViewerServer::forwardNetworkCompleteCallback(MetadosisProtocol::StreamID streamId,NetworkViewerServer::ForwardNetworkCompleteCallbackData cbData)
	{
	/* Access the base client state object and the network viewer client state object: */
	Server::Client* client=server->getClient(cbData.clientId);
	Client* nvClient=client->getPlugin<Client>(pluginIndex);
	
	/* Mark the affected client as having the current network file: */
	nvClient->networkVersion=cbData.networkVersion;
	
	/* Check if the network is done loading; if not, there surely won't be any selected or labeled nodes: */
	if(network!=0)
		{
		/* Send the set of selected nodes to the client: */
		{
		const Network::Selection& selection=network->getSelection();
		Misc::UInt32 numSelectedNodes(selection.getNumEntries());
		MessageWriter selectionSetNotification(NodeSetMsg::createMessage(serverMessageBase+SelectionSetNotification,numSelectedNodes));
		selectionSetNotification.write(networkVersion);
		selectionSetNotification.write(numSelectedNodes);
		for(Network::Selection::ConstIterator sIt=selection.begin();!sIt.isFinished();++sIt)
			selectionSetNotification.write(NodeID(sIt->getSource()));
		client->queueMessage(selectionSetNotification.getBuffer());
		}
		
		/* Send the set of labeled nodes to the client: */
		{
		Misc::UInt32 numLabeledNodes(labeledNodes.getNumEntries());
		MessageWriter labelSetNotification(NodeSetMsg::createMessage(serverMessageBase+LabelSetNotification,numLabeledNodes));
		labelSetNotification.write(networkVersion);
		labelSetNotification.write(numLabeledNodes);
		for(NodeSet::Iterator lnIt=labeledNodes.begin();!lnIt.isFinished();++lnIt)
			labelSetNotification.write(NodeID(lnIt->getSource()));
		client->queueMessage(labelSetNotification.getBuffer());
		}
		}
	}

MessageContinuation* NetworkViewerServer::loadNetworkRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object, its TCP socket, and the network viewer client state object: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	Client* nvClient=client->getPlugin<Client>(pluginIndex);
	
	/* Read the new network version and name and the incoming network file stream ID: */
	Version newNetworkVersion=socket.read<Version>();
	std::string newNetworkName;
	charBufferToString(socket,LoadNetworkMsg::networkNameLen,newNetworkName);
	MetadosisProtocol::StreamID newStreamId=socket.read<MetadosisProtocol::StreamID>();
	
	/* Check that the request's network version matches the current network version: */
	if(newNetworkVersion==networkVersion)
		{
		/* Cancel all clients' active drag operations: */
		for(ClientIDList::iterator cIt=clients.begin();cIt!=clients.end();++cIt)
			server->getPlugin<Client>(*cIt,pluginIndex)->activeDrags.clear();
		
		/* Start loading a new network: */
		do
			{
			++networkVersion;
			}
		while(networkVersion==0);
		delete simulator;
		simulator=0;
		delete network;
		network=0;
		networkFile=0;
		networkName.clear();
		
		/* Start a background job to read the incoming network file: */
		MetadosisProtocol::InStreamPtr newNetworkFile=metadosis->acceptInStream(clientId,newStreamId);
		ReadNetworkJob* job=new ReadNetworkJob(newNetworkName,*newNetworkFile,simulationParameters,*Threads::createFunctionCall(this,&NetworkViewerServer::simulationUpdateCallback),numWorkerThreads);
		Threads::WorkerPool::submitJob(*job,server->getDispatcher(),readNetworkJobCompleteSignalKey);
		
		/* Remember that the requesting client already has the new network file: */
		nvClient->networkVersion=networkVersion;
		
		/* Forward the new network file to all other clients whose metadosis protocols are already initialized: */
		for(ClientIDList::iterator cIt=clients.begin();cIt!=clients.end();++cIt)
			if(*cIt!=clientId)
				{
				/* Access the other client's base client state object: */
				Server::Client* otherClient=server->getClient(*cIt);
				
				/* Send a load network notification with the new network file's stream ID to the other client: */
				{
				MessageWriter loadNetworkNotification(LoadNetworkMsg::createMessage(serverMessageBase+LoadNetworkNotification));
				loadNetworkNotification.write(networkVersion);
				stringToCharBuffer(newNetworkName,loadNetworkNotification,LoadNetworkMsg::networkNameLen);
				loadNetworkNotification.write(metadosis->forwardInStream(*cIt,*newNetworkFile,Threads::createFunctionCall(this,&NetworkViewerServer::forwardNetworkCompleteCallback,ForwardNetworkCompleteCallbackData(*cIt,networkVersion))));
				otherClient->queueMessage(loadNetworkNotification.getBuffer());
				}
				}
		}
	else
		{
		/* Accept the incoming network file stream and immediately drop it to ignore it: */
		metadosis->acceptInStream(clientId,newStreamId,0);
		
		// There should be a way to reject a file in Metadosis, giving an error message to the other end!
		}
	
	/* Done with the message: */
	return 0;
	}

MessageContinuation* NetworkViewerServer::setSimulationParametersRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object and its TCP socket: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	
	/* Read the new simulation parameters: */
	simulationParameters.read(socket);
	
	/* Forward the new simulation parameters to the simulator: */
	if(simulator!=0)
		simulator->setSimulationParameters(simulationParameters);
	
	/* Send the new simulation parameters to all other connected clients: */
	{
	MessageWriter setSimulationParametersNotification(SetSimulationParametersMsg::createMessage(serverMessageBase+SetSimulationParametersNotification));
	simulationParameters.write(setSimulationParametersNotification);
	broadcastMessage(clientId,setSimulationParametersNotification.getBuffer());
	}
	
	/* Done with the message: */
	return 0;
	}

MessageContinuation* NetworkViewerServer::setRenderingParametersRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object and its TCP socket: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	
	/* Read the new rendering parameters: */
	renderingParameters.read(socket);
	
	/* Send the new rendering parameters to all other connected clients: */
	{
	MessageWriter setRenderingParametersNotification(SetRenderingParametersMsg::createMessage(serverMessageBase+SetRenderingParametersNotification));
	renderingParameters.write(setRenderingParametersNotification);
	broadcastMessage(clientId,setRenderingParametersNotification.getBuffer());
	}
	
	/* Done with the message: */
	return 0;
	}

MessageContinuation* NetworkViewerServer::selectNodeRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object and its TCP socket: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	
	/* Read the message: */
	Version version=socket.read<Version>();
	NodeID nodeIndex=socket.read<NodeID>();
	Misc::UInt8 mode=socket.read<Misc::UInt8>();
	
	/* Only process requests applying to the current valid network: */
	if(version==networkVersion&&simulator!=0)
		{
		/* Forward the request to the simulator: */
		simulator->selectNode(nodeIndex,mode);
		
		/* Notify all clients of the change, including the one that requested it: */
		{
		MessageWriter selectNodeNotification(SelectNodeMsg::createMessage(serverMessageBase+SelectNodeNotification));
		selectNodeNotification.write(version);
		selectNodeNotification.write(nodeIndex);
		selectNodeNotification.write(mode);
		broadcastMessage(0,selectNodeNotification.getBuffer());
		}
		}
	
	/* Done with the message: */
	return 0;
	}

MessageContinuation* NetworkViewerServer::changeSelectionRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object and its TCP socket: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	
	/* Read the message: */
	Version version=socket.read<Version>();
	Misc::UInt8 command=socket.read<Misc::UInt8>();
	
	/* Only process requests applying to the current valid network: */
	if(version==networkVersion&&simulator!=0)
		{
		/* Forward the request to the simulator: */
		simulator->changeSelection(command);
		
		/* Notify all clients of the change, including the one that requested it: */
		{
		MessageWriter changeSelectionNotification(ChangeSelectionMsg::createMessage(serverMessageBase+ChangeSelectionNotification));
		changeSelectionNotification.write(version);
		changeSelectionNotification.write(command);
		broadcastMessage(0,changeSelectionNotification.getBuffer());
		}
		}
	
	/* Done with the message: */
	return 0;
	}

MessageContinuation* NetworkViewerServer::displayLabelRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object and its TCP socket: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	
	/* Read the message: */
	Version version=socket.read<Version>();
	NodeID nodeId=socket.read<NodeID>();
	Misc::UInt8 command=socket.read<Misc::UInt8>();
	
	/* Only process requests applying to the current valid network: */
	if(version==networkVersion&&simulator!=0)
		{
		/* Execute the request: */
		switch(command)
			{
			case 0: // Clear label set
				labeledNodes.clear();
				break;
			
			case 1: // Show node label
				labeledNodes.setEntry(NodeSet::Entry(nodeId));
				break;
			
			case 2: // Hide node label
				labeledNodes.removeEntry(nodeId);
				break;
			}
		
		/* Notify all other clients of the change: */
		{
		MessageWriter displayLabelNotification(DisplayLabelMsg::createMessage(serverMessageBase+DisplayLabelNotification));
		displayLabelNotification.write(version);
		displayLabelNotification.write(nodeId);
		displayLabelNotification.write(command);
		broadcastMessage(clientId,displayLabelNotification.getBuffer());
		}
		}
	
	/* Done with the message: */
	return 0;
	}

MessageContinuation* NetworkViewerServer::dragStartRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object, its TCP socket, the network viewer client state object, and the Vrui Core client object: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	Client* nvClient=client->getPlugin<Client>(pluginIndex);
	VruiCoreServer::Client* vcClient=vruiCore->getClient(clientId);
	
	/* Read the message: */
	Version version=socket.read<Version>();
	DragID dragId=socket.read<DragID>();
	VruiCoreProtocol::InputDeviceID inputDeviceId=socket.read<VruiCoreProtocol::InputDeviceID>();
	NodeID nodeIndex=socket.read<NodeID>();
	
	/* Only process requests applying to the current valid network: */
	if(version==networkVersion&&simulator!=0)
		{
		/* Access the input device's Vrui Core state: */
		const VruiCoreProtocol::ClientInputDeviceState& deviceState=vcClient->getDevice(inputDeviceId);
		
		/* Create a new active drag state for the requesting client: */
		nvClient->activeDrags.setEntry(Client::ActiveDragMap::Entry(dragId,Client::ActiveDrag(inputDeviceId,&deviceState)));
		
		/* Calculate the drag transformation in navigational space: */
		VruiCoreProtocol::NavTransform clientNav=vcClient->getNavTransform();
		clientNav.doInvert();
		clientNav*=deviceState.transform;
		NetworkSimulator::DragTransform initialTransform(clientNav.getTranslation(),clientNav.getRotation());
		
		/* Forward the request to the simulator: */
		simulator->dragStart(clientId,dragId,nodeIndex,initialTransform);
		}
	
	/* Done with the message: */
	return 0;
	}

MessageContinuation* NetworkViewerServer::dragRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object, its TCP socket, the network viewer client state object, and the Vrui Core client object: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	Client* nvClient=client->getPlugin<Client>(pluginIndex);
	VruiCoreServer::Client* vcClient=vruiCore->getClient(clientId);
	
	/* Read the message: */
	Version version=socket.read<Version>();
	DragID dragId=socket.read<DragID>();
	
	/* Only process requests applying to the current valid network: */
	if(version==networkVersion&&simulator!=0)
		{
		/* Access the requesting client's active drag state: */
		Client::ActiveDrag& ad=nvClient->activeDrags.getEntry(dragId).getDest();
		
		/* Calculate the drag transformation in navigational space: */
		VruiCoreProtocol::NavTransform clientNav=vcClient->getNavTransform();
		clientNav.doInvert();
		clientNav*=ad.deviceState->transform;
		NetworkSimulator::DragTransform dragTransform(clientNav.getTranslation(),clientNav.getRotation());
		
		/* Forward the request to the simulator: */
		simulator->drag(clientId,dragId,dragTransform);
		}

	/* Done with the message: */
	return 0;
	}

MessageContinuation* NetworkViewerServer::dragStopRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object, its TCP socket, and the network viewer client state object: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	Client* nvClient=client->getPlugin<Client>(pluginIndex);
	
	/* Read the message: */
	Version version=socket.read<Version>();
	DragID dragId=socket.read<DragID>();
	
	/* Only process requests applying to the current valid network: */
	if(version==networkVersion&&simulator!=0)
		{
		/* Remove the active drag state from the requesting client: */
		nvClient->activeDrags.removeEntry(dragId);
		
		/* Forward the request to the simulator: */
		simulator->dragStop(clientId,dragId);
		}
	
	/* Done with the message: */
	return 0;
	}

void NetworkViewerServer::simulationUpdateCallback(const ParticleSystem& particles)
	{
	/* Create a simulation update message: */
	Index numParticles=particles.getNumParticles();
	MessageWriter simulationUpdate(SimulationUpdateMsg::createMessage(serverMessageBase,numParticles));
	simulationUpdate.write(networkVersion);
	simulationUpdate.write(Misc::UInt32(numParticles));
	for(Index i=0;i<numParticles;++i)
		{
		const Point& pos=particles.getParticlePosition(i);
		for(int i=0;i<3;++i)
			simulationUpdate.write(NVScalar(pos[i]));
		}
	
	/* Hand the update message to the main thread to broadcast it to all clients: */
	server->getDispatcher().signal(simulationUpdateSignalKey,simulationUpdate.getBuffer()->ref());
	}

void NetworkViewerServer::sendSimulationUpdateCallback(Threads::EventDispatcher::SignalEvent& event)
	{
	/* Retrieve the simulation update message: */
	MessageBuffer* message=static_cast<MessageBuffer*>(event.getSignalData());
	
	/* Broadcast the message to all connected clients that have received the current network file: */
	for(ClientIDList::iterator cIt=clients.begin();cIt!=clients.end();++cIt)
		{
		/* Access the client's base client state object and its network viewer state object, and check whether it has received the current network file: */
		Server::Client* client=server->getClient(*cIt);
		if(client->getPlugin<Client>(pluginIndex)->networkVersion==networkVersion)
			client->queueMessage(message);
		}
	
	/* Release the message buffer: */
	message->unref();
	}

void NetworkViewerServer::loadNetworkCommandCallback(const char* argumentBegin,const char* argumentEnd)
	{
	#if 0
	/* Check that there is no pending network request: */
	if(uploadingNetworkClientId==0)
		{
		/* Get the network's name and full path: */
		const char* networkNameBegin;
		for(networkNameBegin=argumentEnd;networkNameBegin!=argumentBegin&&networkName[-1]!='/';--networkNameBegin)
			;
		std::string newNetworkName(networkNameBegin,argumentEnd);
		std::string networkPath(argumentBegin,argumentEnd);
		try
			{
			/* Open the requested network file: */
			IO::FilePtr file=IO::openFile(networkPath.c_str());
			
			/* Cancel all clients' active drag operations: */
			for(ClientIDList::iterator cIt=clients.begin();cIt!=clients.end();++cIt)
				server->getPlugin<Client>(*cIt,pluginIndex)->activeDrags.clear();
			
			/* Start loading a new network: */
			uploadingNetworkClientId=-1U;
			do
				{
				++networkVersion;
				}
			while(networkVersion==0);
			delete simulator;
			simulator=0;
			delete network;
			network=0;
			networkFile=0;
			networkName.clear();
			
			/* Start a background job to read the incoming network file: */
			ReadNetworkJob* job=new ReadNetworkJob(newNetworkName,*file,simulationParameters,*Threads::createFunctionCall(this,&NetworkViewerServer::simulationUpdateCallback),numWorkerThreads);
			Threads::WorkerPool::submitJob(*job,server->getDispatcher(),readNetworkJobCompleteSignalKey);
			
			/* Forward the new network file to all clients whose metadosis protocols are already initialized: */
			for(ClientIDList::iterator cIt=clients.begin();cIt!=clients.end();++cIt)
				{
				/* Access the other client's base client state object and its network viewer state object, and check whether its metadosis protocol is initialized: */
				Server::Client* client=server->getClient(*cIt);
				Client* nvClient=client->getPlugin<Client>(pluginIndex);
				if(nvClient->hasMetadosis)
					{
					/* Send a load network notification with the new network file's stream ID to the client: */
					MessageWriter loadNetworkNotification(LoadNetworkMsg::createMessage(serverMessageBase+LoadNetworkNotification));
					loadNetworkNotification.write(networkVersion);
					stringToCharBuffer(networkName,loadNetworkNotification,LoadNetworkMsg::networkNameLen);
					loadNetworkNotification.write(metadosis->forwardInStream(*cIt,*networkFile,Threads::createFunctionCall(this,&NetworkViewerServer::forwardNetworkCompleteCallback,ForwardNetworkCompleteCallbackData(*cIt,networkVersion))));
					client->queueMessage(loadNetworkNotification.getBuffer());
					}
				}
			}
		catch(const std::runtime_error& err)
			{
			/* Print an error message: */
			Misc::formattedUserError("NetworkViewerServer: Cannot load network %s due to exception %s",networkPath.c_str(),err.what());
			}
		}
	else
		{
		/* Print an error message: */
		Misc::userError("NetworkViewerServer: Cannot load network because there is already a pending load network request");
		}
	#endif
	}

NetworkViewerServer::NetworkViewerServer(Server* sServer)
	:PluginServer(sServer),
	 metadosis(MetadosisServer::requestServer(server)),
	 vruiCore(VruiCoreServer::requestServer(server)),
	 networkVersion(0),network(0),labeledNodes(17),
	 numWorkerThreads(3),simulator(0)
	{
	/* Register dependencies with the service protocol plug-ins: */
	metadosis->addDependentPlugin(this);
	vruiCore->addDependentPlugin(this);
	
	/* Register signals with the server's event dispatcher: */
	readNetworkJobCompleteSignalKey=server->getDispatcher().addSignalListener(Threads::EventDispatcher::wrapMethod<NetworkViewerServer,&NetworkViewerServer::readNetworkJobCompleteCallback>,this);
	simulationUpdateSignalKey=server->getDispatcher().addSignalListener(Threads::EventDispatcher::wrapMethod<NetworkViewerServer,&NetworkViewerServer::sendSimulationUpdateCallback>,this);
	
	/* Register pipe commands: */
	server->getCommandDispatcher().addCommandCallback("NetworkViewer::loadNetwork",Misc::CommandDispatcher::wrapMethod<NetworkViewerServer,&NetworkViewerServer::loadNetworkCommandCallback>,this,"<network file name>","Loads the network file of the given name");
	}

NetworkViewerServer::~NetworkViewerServer(void)
	{
	/* Shut down the simulator and delete the network: */
	delete simulator;
	delete network;
	
	/* Remove event dispatcher signals: */
	server->getDispatcher().removeSignalListener(readNetworkJobCompleteSignalKey);
	server->getDispatcher().removeSignalListener(simulationUpdateSignalKey);
	
	/* Release dependencies from the service protocol plug-ins: */
	vruiCore->removeDependentPlugin(this);
	metadosis->removeDependentPlugin(this);
	}

const char* NetworkViewerServer::getName(void) const
	{
	return protocolName;
	}

unsigned int NetworkViewerServer::getVersion(void) const
	{
	return protocolVersion;
	}

unsigned int NetworkViewerServer::getNumClientMessages(void) const
	{
	return NumClientMessages;
	}

unsigned int NetworkViewerServer::getNumServerMessages(void) const
	{
	return NumServerMessages;
	}

void NetworkViewerServer::setMessageBases(unsigned int newClientMessageBase,unsigned int newServerMessageBase)
	{
	/* Call the base class method: */
	PluginServer::setMessageBases(newClientMessageBase,newServerMessageBase);
	
	/* Register message handlers: */
	server->setMessageHandler(clientMessageBase+LoadNetworkRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::loadNetworkRequestCallback>,this,LoadNetworkMsg::size);
	server->setMessageHandler(clientMessageBase+SetSimulationParametersRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::setSimulationParametersRequestCallback>,this,SetSimulationParametersMsg::size);
	server->setMessageHandler(clientMessageBase+SetRenderingParametersRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::setRenderingParametersRequestCallback>,this,SetRenderingParametersMsg::size);
	server->setMessageHandler(clientMessageBase+SelectNodeRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::selectNodeRequestCallback>,this,SelectNodeMsg::size);
	server->setMessageHandler(clientMessageBase+ChangeSelectionRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::changeSelectionRequestCallback>,this,ChangeSelectionMsg::size);
	server->setMessageHandler(clientMessageBase+DisplayLabelRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::displayLabelRequestCallback>,this,DisplayLabelMsg::size);
	server->setMessageHandler(clientMessageBase+DragStartRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::dragStartRequestCallback>,this,DragStartRequestMsg::size);
	server->setMessageHandler(clientMessageBase+DragRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::dragRequestCallback>,this,DragRequestMsg::size);
	server->setMessageHandler(clientMessageBase+DragStopRequest,Server::wrapMethod<NetworkViewerServer,&NetworkViewerServer::dragStopRequestCallback>,this,DragStopRequestMsg::size);
	}

void NetworkViewerServer::start(void)
	{
	}

void NetworkViewerServer::clientConnected(unsigned int clientId)
	{
	/* Unpause the simulator if there is one, and this is the first client to connect : */
	if(simulator!=0&&clients.empty())
		{
		Misc::logNote("NetworkViewer: Unpausing simulation thread");
		simulator->resume();
		}
	
	/* Call the base class method: */
	PluginServer::clientConnected(clientId);
	
	/* Associate a client structure with the new client: */
	Server::Client* client=server->getClient(clientId);
	client->setPlugin(pluginIndex,new Client);
	
	/* Send the current simulation and rendering parameters to the new client: */
	{
	MessageWriter setSimulationParametersNotification(SetSimulationParametersMsg::createMessage(serverMessageBase+SetSimulationParametersNotification));
	simulationParameters.write(setSimulationParametersNotification);
	client->queueMessage(setSimulationParametersNotification.getBuffer());
	}
	{
	MessageWriter setRenderingParametersNotification(SetRenderingParametersMsg::createMessage(serverMessageBase+SetRenderingParametersNotification));
	renderingParameters.write(setRenderingParametersNotification);
	client->queueMessage(setRenderingParametersNotification.getBuffer());
	}
	
	/* Check if there is a network file: */
	if(networkFile!=0)
		{
		/* Forward the network file to the new client: */
		MessageWriter loadNetworkNotification(LoadNetworkMsg::createMessage(serverMessageBase+LoadNetworkNotification));
		loadNetworkNotification.write(networkVersion);
		stringToCharBuffer(networkName,loadNetworkNotification,LoadNetworkMsg::networkNameLen);
		loadNetworkNotification.write(metadosis->forwardInStream(clientId,*networkFile,Threads::createFunctionCall(this,&NetworkViewerServer::forwardNetworkCompleteCallback,ForwardNetworkCompleteCallbackData(clientId,networkVersion))));
		client->queueMessage(loadNetworkNotification.getBuffer());
		}
	}

void NetworkViewerServer::clientDisconnected(unsigned int clientId)
	{
	/* Access the network viewer client state object: */
	Client* nvClient=server->getPlugin<Client>(clientId,pluginIndex);
	
	/* Check if there is a simulator: */
	if(simulator!=0)
		{
		/* Remove the client's potentially remaining active drags: */
		for(Client::ActiveDragMap::Iterator adIt=nvClient->activeDrags.begin();!adIt.isFinished();++adIt)
			simulator->dragStop(clientId,adIt->getSource());
		}
	
	/* Call the base class method: */
	PluginServer::clientDisconnected(clientId);
	
	/* Pause the simulator if there is one and this is the last client to disconnect: */
	if(simulator!=0&&clients.empty())
		{
		Misc::logNote("NetworkViewer: Pausing simulation thread");
		simulator->pause();
		}
	}

/***********************
DSO loader entry points:
***********************/

extern "C" {

PluginServer* createObject(PluginServerLoader& objectLoader,Server* server)
	{
	return new NetworkViewerServer(server);
	}

void destroyObject(PluginServer* object)
	{
	delete object;
	}

}

}

}
