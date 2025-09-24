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

#include "NetworkViewerClient.h"

#include <Misc/Autopointer.h>
#include <Misc/MessageLogger.h>
#include <Threads/FunctionCalls.h>
#include <Threads/WorkerPool.h>
#include <IO/OpenFile.h>
#include <Collaboration2/MessageReader.h>
#include <Collaboration2/MessageWriter.h>
#include <Collaboration2/MessageContinuation.h>

#include "Network.h"
#include "CollaborativeNetworkViewer.h"

namespace Collab {

namespace Plugins {

/****************************************************
Methods of class NetworkViewerClient::ReadNetworkJob:
****************************************************/

NetworkViewerClient::ReadNetworkJob::ReadNetworkJob(unsigned int sNetworkVersion,const std::string& sNetworkName,IO::File& sNetworkFile,Client* sClient,unsigned int sServerMessageBase)
	:networkVersion(sNetworkVersion),networkName(sNetworkName),networkFile(&sNetworkFile),
	 network(0),
	 client(sClient),serverMessageBase(sServerMessageBase)
	{
	}

NetworkViewerClient::ReadNetworkJob::~ReadNetworkJob(void)
	{
	/* Release allocated resources: */
	delete network;
	}
	
void NetworkViewerClient::ReadNetworkJob::operator()(int) const
	{
	/* Can't really do anything; this is an API problem that should be fixed: */
	// ...
	}

void NetworkViewerClient::ReadNetworkJob::operator()(int)
	{
	try
		{
		/* Parse the network file into a network object: */
		network=new Network(*networkFile);
		}
	catch(const std::runtime_error& err)
		{
		/* Show an error message: */
		Misc::formattedUserError("NetworkViewer::ReadNetworkJob: Unable to read network %s due to exception %s",networkName.c_str(),err.what());
		}
	
	/* Send a completion message to the frontend: */
	{
	MessageWriter loadNetworkCompleteNotification(MessageBuffer::create(serverMessageBase+LoadNetworkCompleteNotification,sizeof(ReadNetworkJob*)));
	ref();
	loadNetworkCompleteNotification.write(this);
	client->queueFrontendMessage(loadNetworkCompleteNotification.getBuffer());
	}
	}
	
/************************************
Methods of class NetworkViewerClient:
************************************/

void NetworkViewerClient::loadNetworkCompleteNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the job object pointer: */
	ReadNetworkJob* job=message.read<ReadNetworkJob*>();
	
	/* Check if this network is still the most recent one being downloaded: */
	if(job->networkVersion==downloadingVersion)
		{
		/* Replace the current network: */
		networkVersion=job->networkVersion;
		networkName=job->networkName;
		delete network;
		network=job->network;
		job->network=0;
		
		/* Check if there is a selection set to be applied to the new network: */
		if(selectionSet!=0)
			{
			/* Select the nodes contained in the selection set: */
			for(std::vector<NodeID>::iterator ssIt=selectionSet->begin();ssIt!=selectionSet->end();++ssIt)
				network->selectNode(*ssIt);
			
			/* Release the selection set: */
			delete selectionSet;
			selectionSet=0;
			}
		
		/* Check if there is a label set to be applied to the new network: */
		if(labelSet!=0)
			{
			/* Label the nodes contained in the selection set: */
			for(std::vector<NodeID>::iterator lsIt=labelSet->begin();lsIt!=labelSet->end();++lsIt)
				application->showNodeLabel(*lsIt);
			
			/* Release the label set: */
			delete labelSet;
			labelSet=0;
			}
		
		/* Mark the application's network structures as outdated: */
		application->updateNetwork(network);
		}
	
	/* Release the job object: */
	job->unref();
	}

void NetworkViewerClient::loadNetworkNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the new network's name and the stream ID of the new network file: */
	Version newNetworkVersion=message.read<Version>();
	std::string newNetworkName;
	charBufferToString(message,LoadNetworkMsg::networkNameLen,newNetworkName);
	MetadosisProtocol::StreamID streamId=message.read<MetadosisProtocol::StreamID>();
	
	/* Set the version of the currently downloading network: */
	downloadingVersion=newNetworkVersion;
	
	/* Clear any lingering unapplied selection and label sets: */
	delete selectionSet;
	selectionSet=0;
	delete labelSet;
	labelSet=0;
	
	/* Clear all current node labels: */
	application->clearNodeLabels();
	
	/* Notify the user that a new network is being loaded: */
	Misc::formattedUserNote("NetworkViewer: Loading new network %s",newNetworkName.c_str());
	
	/* Start a background job to read the incoming network file: */
	ReadNetworkJob* job=new ReadNetworkJob(newNetworkVersion,newNetworkName,*metadosis->acceptInStream(streamId),client,serverMessageBase);
	Threads::WorkerPool::submitJob(*job);
	}

void NetworkViewerClient::selectionSetNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the network version of the selection set and the number of selected nodes: */
	Version selectionSetVersion=message.read<Version>();
	size_t numSelectedNodes=message.read<Misc::UInt32>();
	
	/* Check if the selection set's version matches the current network version: */
	if(selectionSetVersion==networkVersion)
		{
		/* Apply the selection set to the loaded network: */
		network->clearSelection();
		for(size_t i=0;i<numSelectedNodes;++i)
			network->selectNode(message.read<NodeID>());
		}
	else if(selectionSetVersion==downloadingVersion)
		{
		/* Save the selection set to be applied once the downloading network finished processing: */
		delete selectionSet;
		selectionSet=new std::vector<NodeID>;
		selectionSet->reserve(numSelectedNodes);
		for(size_t i=0;i<numSelectedNodes;++i)
			selectionSet->push_back(message.read<NodeID>());
		}
	else
		{
		/* Just ignore the selection set */
		}
	}

void NetworkViewerClient::labelSetNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the network version of the label set and the number of labeled nodes: */
	Version labelSetVersion=message.read<Version>();
	size_t numLabeledNodes=message.read<Misc::UInt32>();
	
	/* Check if the label set's version matches the current network version: */
	if(labelSetVersion==networkVersion)
		{
		/* Apply the label set to the loaded network: */
		application->clearNodeLabels();
		for(size_t i=0;i<numLabeledNodes;++i)
			application->showNodeLabel(message.read<NodeID>());
		}
	else if(labelSetVersion==downloadingVersion)
		{
		/* Save the label set to be applied once the downloading network finished processing: */
		delete labelSet;
		labelSet=new std::vector<NodeID>;
		labelSet->reserve(numLabeledNodes);
		for(size_t i=0;i<numLabeledNodes;++i)
			labelSet->push_back(message.read<NodeID>());
		}
	else
		{
		/* Just ignore the label set */
		}
	}

void NetworkViewerClient::setSimulationParametersNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the new simulation parameters and notify the application of the change: */
	SimulationParameters simulationParameters;
	simulationParameters.read(message);
	application->updateSimulationParameters(simulationParameters);
	}

void NetworkViewerClient::setRenderingParametersNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the new rendering parameters and notify the application of the change: */
	RenderingParameters renderingParameters;
	renderingParameters.read(message);
	application->updateRenderingParameters(renderingParameters);
	}

void NetworkViewerClient::selectNodeNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the message: */
	Version msgNetworkVersion=message.read<Version>();
	NodeID node=message.read<NodeID>();
	Misc::UInt8 mode=message.read<Misc::UInt8>();
	
	/* Check if the network version matches: */
	if(msgNetworkVersion==networkVersion)
		{
		/* Change the node's selection state: */
		switch(mode)
			{
			case 0: // Select node
				network->selectNode(node);
				break;
			
			case 1: // Deselect node
				network->deselectNode(node);
				break;
			
			case 2: // Toggle node's selection state
				if(network->isSelected(node))
					network->deselectNode(node);
				else
					network->selectNode(node);
				break;
			}
		}
	}

void NetworkViewerClient::changeSelectionNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the message: */
	Version msgNetworkVersion=message.read<Version>();
	Misc::UInt8 command=message.read<Misc::UInt8>();
	
	/* Check if the network version matches: */
	if(msgNetworkVersion==networkVersion)
		{
		/* Change the network's selection set: */
		switch(command)
			{
			case 0: // Clear selection
				network->clearSelection();
				break;
			
			case 1: // Grow selection
				network->growSelection();
				break;
			
			case 2: // Shrink selection
				network->shrinkSelection();
				break;
			}
		}
	}

void NetworkViewerClient::displayLabelNotificationCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the message: */
	Version msgNetworkVersion=message.read<Version>();
	NodeID nodeId=message.read<NodeID>();
	Misc::UInt8 command=message.read<Misc::UInt8>();
	
	/* Check if the network version matches: */
	if(msgNetworkVersion==networkVersion)
		{
		/* Change the network's label set: */
		switch(command)
			{
			case 0: // Clear the label set
				application->clearNodeLabels();
				break;
			
			case 1: // Show node label
				application->showNodeLabel(nodeId);
				break;
			
			case 2: // Hide node label
				application->hideNodeLabel(nodeId);
				break;
			}
		}
	}

MessageContinuation* NetworkViewerClient::simulationUpdateCallback(unsigned int messageId,MessageContinuation* continuation)
	{
	/* Embedded classes: */
	class Cont:public MessageContinuation
		{
		/* Elements: */
		public:
		Version networkVersion; // Version number of the network to which this update applies
		size_t numParticles; // Number of unread particle positions in the message
		NVPointList& points; // Point array into which particle positions are being read
		
		/* Constructors and destructors: */
		Cont(Version sNetworkVersion,size_t sNumParticles,NVPointList& sPoints)
			:networkVersion(sNetworkVersion),numParticles(sNumParticles),points(sPoints)
			{
			}
		};
	
	NonBlockSocket& socket=client->getSocket();
	
	/* Check if this is the start of a new message: */
	Cont* cont=static_cast<Cont*>(continuation);
	if(cont==0)
		{
		/* Read the network version number: */
		Version networkVersion=socket.read<Version>();
		
		/* Read the number of particle positions contained in the message: */
		size_t numParticles=socket.read<Misc::UInt32>();
		
		/* Start a new value in the point positions triple buffer: */
		NVPointList& points=application->positions.startNewValue();
		points.clear();
		points.reserve(numParticles);
		
		/* Create a continuation object: */
		cont=new Cont(networkVersion,numParticles,points);
		}
	
	/* Read as many particle positions as are available: */
	size_t readParticles=Misc::min(cont->numParticles,socket.getUnread()/pointSize);
	cont->numParticles-=readParticles;
	while(readParticles>0)
		{
		/* Read a particle position: */
		Point ppos;
		for(int i=0;i<3;++i)
			ppos[i]=Scalar(socket.read<NVScalar>());
		cont->points.push_back(ppos);
		--readParticles;
		}
	
	/* Check if the update is complete: */
	if(cont->numParticles==0)
		{
		/* Check if the update's network version matches the current network: */
		if(cont->networkVersion==networkVersion)
			{
			/* Post the new point positions value: */
			application->positions.postNewValue();
			application->networkPositionVersion=application->networkVersion;
			Vrui::requestUpdate();
			}
		
		/* Done with the message: */
		delete cont;
		cont=0;
		}
	
	return cont;
	}

NetworkViewerClient::NetworkViewerClient(CollaborativeNetworkViewer* sApplication,Client* sClient)
	:PluginClient(sClient),
	 application(sApplication),
	 metadosis(MetadosisClient::requestClient(client)),
	 networkVersion(0),network(0),downloadingVersion(0),selectionSet(0),labelSet(0),
	 lastDragId(0),activeDrags(5)
	{
	}

NetworkViewerClient::~NetworkViewerClient(void)
	{
	/* Release allocated resources: */
	delete network;
	delete selectionSet;
	delete labelSet;
	}

const char* NetworkViewerClient::getName(void) const
	{
	return protocolName;
	}

unsigned int NetworkViewerClient::getVersion(void) const
	{
	return protocolVersion;
	}

unsigned int NetworkViewerClient::getNumClientMessages(void) const
	{
	return NumClientMessages;
	}

unsigned int NetworkViewerClient::getNumServerMessages(void) const
	{
	return NumServerMessages;
	}

void NetworkViewerClient::setMessageBases(unsigned int newClientMessageBase,unsigned int newServerMessageBase)
	{
	/* Call the base class method: */
	PluginClient::setMessageBases(newClientMessageBase,newServerMessageBase);
	
	/* Register message handlers: */
	client->setMessageForwarder(serverMessageBase+LoadNetworkNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::loadNetworkNotificationCallback>,this,LoadNetworkMsg::size);
	client->setFrontendMessageHandler(serverMessageBase+LoadNetworkCompleteNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::loadNetworkCompleteNotificationCallback>,this);
	client->setVariableSizeMessageForwarder(serverMessageBase+SelectionSetNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::selectionSetNotificationCallback>,this,NodeSetMsg::size,Client::UInt32,sizeof(NodeID));
	client->setVariableSizeMessageForwarder(serverMessageBase+LabelSetNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::labelSetNotificationCallback>,this,NodeSetMsg::size,Client::UInt32,sizeof(NodeID));
	client->setMessageForwarder(serverMessageBase+SetSimulationParametersNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::setSimulationParametersNotificationCallback>,this,SetSimulationParametersMsg::size);
	client->setMessageForwarder(serverMessageBase+SetRenderingParametersNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::setRenderingParametersNotificationCallback>,this,SetRenderingParametersMsg::size);
	client->setMessageForwarder(serverMessageBase+SelectNodeNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::selectNodeNotificationCallback>,this,SelectNodeMsg::size);
	client->setMessageForwarder(serverMessageBase+ChangeSelectionNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::changeSelectionNotificationCallback>,this,ChangeSelectionMsg::size);
	client->setMessageForwarder(serverMessageBase+DisplayLabelNotification,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::displayLabelNotificationCallback>,this,DisplayLabelMsg::size);
	client->setTCPMessageHandler(serverMessageBase+SimulationUpdate,Client::wrapMethod<NetworkViewerClient,&NetworkViewerClient::simulationUpdateCallback>,this,SimulationUpdateMsg::size);
	}

void NetworkViewerClient::start(void)
	{
	#if 0
	if(loadNetworkFileName!=0)
		{
		try
			{
			/* Open the requested file: */
			IO::FilePtr networkFile=IO::openFile(loadNetworkFileName);
			
			/* Send the requested file to the server: */
			{
			MessageWriter loadNetworkRequest(LoadNetworkMsg::createMessage(clientMessageBase+LoadNetworkRequest));
			stringToCharBuffer(Misc::getFileName(loadNetworkFileName),loadNetworkRequest,LoadNetworkMsg::networkNameLen);
			client->queueMessage(loadNetworkRequest.getBuffer());
			}
			
			while(!networkFile->eof())
				{
				/* Send a chunk of the network file to the server: */
				MessageWriter loadNetworkRequestChunk(LoadNetworkChunkMsg::createMessage(clientMessageBase+LoadNetworkRequestChunk));
				
				/* Write the maximum chunk size to the message buffer: */
				loadNetworkRequestChunk.write(Misc::UInt16(LoadNetworkChunkMsg::maxChunkSize));
				
				/* Write a chunk of data to the message buffer: */
				size_t chunkSize=networkFile->readUpTo(loadNetworkRequestChunk.getWritePtr(),LoadNetworkChunkMsg::maxChunkSize);
				loadNetworkRequestChunk.advanceWritePtr(chunkSize);
				loadNetworkRequestChunk.finishMessage();
				
				/* Write the actual chunk size to the message buffer: */
				loadNetworkRequestChunk.rewind();
				loadNetworkRequestChunk.write(Misc::UInt16(chunkSize));
				
				client->queueMessage(loadNetworkRequestChunk.getBuffer());
				}
			
			/* Write a zero-length chunk to signal the end of the file: */
			{
			MessageWriter loadNetworkRequestChunk(LoadNetworkChunkMsg::createMessage(clientMessageBase+LoadNetworkRequestChunk));
			loadNetworkRequestChunk.write(Misc::UInt16(0));
			loadNetworkRequestChunk.finishMessage();
			client->queueMessage(loadNetworkRequestChunk.getBuffer());
			}
			}
		catch(const std::runtime_error& err)
			{
			Misc::formattedUserError("NetworkViewerClient::start: Caught exception %s",err.what());
			}
		}
	#endif
	}

void NetworkViewerClient::loadNetwork(const char* networkFileName)
	{
	/* Get the network's name: */
	const char* networkNameBegin=networkFileName;
	for(const char* nfnPtr=networkFileName;*nfnPtr!='\0';++nfnPtr)
		if(*nfnPtr=='/')
			networkNameBegin=nfnPtr+1;
	std::string newNetworkName(networkNameBegin);
	
	/* Open the requested file: */
	IO::FilePtr networkFile=IO::openFile(networkFileName);
	
	/* Wrap the file in a forwarding filter to upload it to the server while it is being read: */
	MetadosisClient::ForwardingFilterPtr forwardingFilter=metadosis->forwardFile(*networkFile);
	
	/* Send a load network request to the server: */
	{
	MessageWriter loadNetworkRequest(LoadNetworkMsg::createMessage(clientMessageBase+LoadNetworkRequest));
	loadNetworkRequest.write(networkVersion);
	stringToCharBuffer(newNetworkName,loadNetworkRequest,LoadNetworkMsg::networkNameLen);
	loadNetworkRequest.write(forwardingFilter->getStreamId());
	client->queueServerMessage(loadNetworkRequest.getBuffer());
	}
	
	/* Clear all current node labels: */
	application->clearNodeLabels();
	
	/* Notify the user that a new network is being loaded: */
	Misc::formattedUserNote("NetworkViewer: Loading new network %s",newNetworkName.c_str());
	
	/* Get a new network version number: */
	do
		{
		++networkVersion;
		}
	while(networkVersion==0);
	
	/* Start a background job to read the incoming network file: */
	downloadingVersion=networkVersion;
	delete selectionSet;
	selectionSet=0;
	ReadNetworkJob* job=new ReadNetworkJob(networkVersion,newNetworkName,*forwardingFilter,client,serverMessageBase);
	Threads::WorkerPool::submitJob(*job);
	}

void NetworkViewerClient::updateSimulationParameters(const SimulationParameters& simulationParameters)
	{
	/* Send the current simulation parameters to the server: */
	MessageWriter setSimulationParametersRequest(SetSimulationParametersMsg::createMessage(clientMessageBase+SetSimulationParametersRequest));
	simulationParameters.write(setSimulationParametersRequest);
	client->queueServerMessage(setSimulationParametersRequest.getBuffer());
	}

void NetworkViewerClient::selectNode(unsigned int pickedNodeIndex,unsigned int mode)
	{
	/* Send a select node request to the server: */
	MessageWriter selectNodeRequest(SelectNodeMsg::createMessage(clientMessageBase+SelectNodeRequest));
	selectNodeRequest.write(networkVersion);
	selectNodeRequest.write(NodeID(pickedNodeIndex));
	selectNodeRequest.write(Misc::UInt8(mode));
	client->queueServerMessage(selectNodeRequest.getBuffer());
	}

void NetworkViewerClient::changeSelection(unsigned int command)
	{
	/* Send a change selection request to the server: */
	MessageWriter changeSelectionRequest(ChangeSelectionMsg::createMessage(clientMessageBase+ChangeSelectionRequest));
	changeSelectionRequest.write(networkVersion);
	changeSelectionRequest.write(Misc::UInt8(command));
	client->queueServerMessage(changeSelectionRequest.getBuffer());
	}

void NetworkViewerClient::displayLabel(unsigned int nodeIndex,unsigned int command)
	{
	/* Send a display label request to the server: */
	MessageWriter displayLabelRequest(DisplayLabelMsg::createMessage(clientMessageBase+DisplayLabelRequest));
	displayLabelRequest.write(networkVersion);
	displayLabelRequest.write(NodeID(nodeIndex));
	displayLabelRequest.write(Misc::UInt8(command));
	client->queueServerMessage(displayLabelRequest.getBuffer());
	}

unsigned int NetworkViewerClient::startDrag(unsigned int inputDeviceId,unsigned int pickedNodeIndex)
	{
	/* Retrieve the next unused drag ID: */
	do
		{
		++lastDragId;
		}
	while(lastDragId==0||activeDrags.isEntry(lastDragId));
	
	/* Mark the found drag ID as active: */
	activeDrags.setEntry(ActiveDragSet::Entry(lastDragId));
	
	/* Send a drag start request to the server: */
	{
	MessageWriter dragStartRequest(DragStartRequestMsg::createMessage(clientMessageBase));
	dragStartRequest.write(networkVersion);
	dragStartRequest.write(DragID(lastDragId));
	dragStartRequest.write(VruiCoreProtocol::InputDeviceID(inputDeviceId));
	dragStartRequest.write(NodeID(pickedNodeIndex));
	client->queueServerMessage(dragStartRequest.getBuffer());
	}
	
	return lastDragId;
	}

void NetworkViewerClient::drag(unsigned int dragId)
	{
	/* Send a drag request to the server: */
	MessageWriter dragRequest(DragRequestMsg::createMessage(clientMessageBase));
	dragRequest.write(networkVersion);
	dragRequest.write(DragID(dragId));
	client->queueServerMessage(dragRequest.getBuffer());
	}

void NetworkViewerClient::stopDrag(unsigned int dragId)
	{
	/* Find the given drag ID in the active drag set: */
	ActiveDragSet::Iterator adIt=activeDrags.findEntry(dragId);
	if(!adIt.isFinished())
		{
		/* Send a drag stop request to the server: */
		{
		MessageWriter dragStopRequest(DragStopRequestMsg::createMessage(clientMessageBase));
		dragStopRequest.write(networkVersion);
		dragStopRequest.write(DragID(dragId));
		client->queueServerMessage(dragStopRequest.getBuffer());
		}
		
		/* Mark the drag ID as inactive: */
		activeDrags.removeEntry(adIt);
		}
	}

void NetworkViewerClient::updateRenderingParameters(const RenderingParameters& renderingParameters)
	{
	/* Send the current rendering parameters to the server: */
	MessageWriter setRenderingParametersRequest(SetRenderingParametersMsg::createMessage(clientMessageBase+SetRenderingParametersRequest));
	renderingParameters.write(setRenderingParametersRequest);
	client->queueServerMessage(setRenderingParametersRequest.getBuffer());
	}

}

}
