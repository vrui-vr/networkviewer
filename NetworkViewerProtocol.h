/***********************************************************************
NetworkViewerProtocol - Definition of the communication protocol between
a network viewer client and a server.
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

#ifndef NETWORKVIEWERPROTOCOL_INCLUDED
#define NETWORKVIEWERPROTOCOL_INCLUDED

#include <string>
#include <Misc/SizedTypes.h>
#include <Geometry/Point.h>
#include <Collaboration2/Protocol.h>
#include <Collaboration2/MessageBuffer.h>
#include <Collaboration2/Plugins/MetadosisProtocol.h>
#include <Collaboration2/Plugins/VruiCoreProtocol.h>

#include "ParticleTypes.h"
#include "SimulationParameters.h"
#include "RenderingParameters.h"

namespace Collab {

namespace Plugins {

class NetworkViewerProtocol
	{
	/* Embedded classes: */
	protected:
	
	/* Protocol message IDs: */
	enum ClientMessages // Enumerated type for network viewer protocol message IDs sent by clients
		{
		LoadNetworkRequest=0,
		SetSimulationParametersRequest,
		SetRenderingParametersRequest,
		SelectNodeRequest,
		ChangeSelectionRequest,
		DisplayLabelRequest,
		DragStartRequest,
		DragRequest,
		DragStopRequest,
		NumClientMessages
		};
	
	enum ServerMessages // Enumerated type for network viewer protocol message IDs sent by servers
		{
		LoadNetworkNotification=0,
		LoadNetworkCompleteNotification, // Not actually sent from the server, but from a background thread on the client
		SelectionSetNotification,
		LabelSetNotification,
		SetSimulationParametersNotification,
		SetRenderingParametersNotification,
		SelectNodeNotification,
		ChangeSelectionNotification,
		DisplayLabelNotification,
		SimulationUpdate,
		NumServerMessages
		};
	
	/* Protocol data type declarations: */
	public:
	typedef Misc::UInt16 Version; // Type for network version numbers
	typedef Misc::Float32 NVScalar; // Scalar type for 3D geometry
	static const size_t scalarSize=sizeof(NVScalar); // Wire size of a scalar
	typedef Geometry::Point<NVScalar,3> NVPoint; // Type for affine points
	static const size_t pointSize=3*scalarSize; // Wire size of an affine point
	typedef Misc::UInt32 NodeID; // Type to identify network nodes
	typedef Misc::UInt16 DragID; // Type to identify drag sequences
	
	/* Protocol message data structure declarations: */
	protected:
	struct LoadNetworkMsg
		{
		/* Elements: */
		public:
		static const size_t networkNameLen=256; // Maximum length of network names
		static const size_t size=sizeof(Version)+networkNameLen*sizeof(Char)+sizeof(MetadosisProtocol::StreamID);
		Version networkVersion; // Version number for the new network
		Char networkName[networkNameLen]; // Name of the new network
		MetadosisProtocol::StreamID streamId; // Stream ID of the Metadosis stream from which the new network file can be read
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageId) // Returns a message buffer for a load network request or notification message
			{
			return MessageBuffer::create(messageId,size);
			}
		};
	
	struct NodeSetMsg
		{
		/* Elements: */
		public:
		static const size_t size=sizeof(Version)+sizeof(Misc::UInt32); // Size of fixed portion of message
		Version networkVersion; // Version number for the new network
		Misc::UInt32 numNodes; // Number of nodes in the set
		// NodeID nodes[numNodes]; // Array of IDs of nodes in the set
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageId,unsigned int numNodes) // Returns a message buffer for a selection set notification or label set notification message for the given number of nodes in the set
			{
			return MessageBuffer::create(messageId,size+numNodes*sizeof(NodeID));
			}
		};
	
	struct SetSimulationParametersMsg
		{
		/* Elements: */
		public:
		static const size_t size=SimulationParameters::size;
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageId) // Returns a message buffer for a set simulation parameters request or notification message
			{
			return MessageBuffer::create(messageId,SimulationParameters::size);
			}
		};
	
	struct SetRenderingParametersMsg
		{
		/* Elements: */
		public:
		static const size_t size=RenderingParameters::size;
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageId) // Returns a message buffer for a set rendering parameters request or notification message
			{
			return MessageBuffer::create(messageId,RenderingParameters::size);
			}
		};
	
	struct SelectNodeMsg
		{
		/* Elements: */
		public:
		static const size_t size=sizeof(Version)+sizeof(NodeID)+sizeof(Misc::UInt8);
		Version networkVersion; // Version number of the network to which the message applies
		NodeID node; // The node whose selection state is to be changed
		Misc::UInt8 mode; // Selection mode: 0: add to selection; 1: remove from selection, 2: toggle selection state
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageId) // Returns a message buffer for a select node request or notification message
			{
			return MessageBuffer::create(messageId,size);
			}
		};
	
	struct ChangeSelectionMsg
		{
		/* Elements: */
		public:
		static const size_t size=sizeof(Version)+sizeof(Misc::UInt8);
		Version networkVersion; // Version number of the network to which the message applies
		Misc::UInt8 command; // Change selection command: 0 - clear selection, 1 - grow selection, 2 - shrink selection
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageId) // Returns a message buffer for a change selection request or notification message
			{
			return MessageBuffer::create(messageId,size);
			}
		};
	
	struct DisplayLabelMsg
		{
		/* Elements: */
		public:
		static const size_t size=sizeof(Version)+sizeof(NodeID)+sizeof(Misc::UInt8);
		Version networkVersion; // Version number of the network to which the message applies
		NodeID node; // The node whose label is to be changed
		Misc::UInt8 command; // Display label command: 0 - clear label set, 1 - show node label, 2 - hide node label
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageId) // Returns a message buffer for a display label request or notification message
			{
			return MessageBuffer::create(messageId,size);
			}
		};
	
	struct DragStartRequestMsg
		{
		/* Elements: */
		public:
		static const size_t size=sizeof(Version)+sizeof(DragID)+sizeof(VruiCoreProtocol::InputDeviceID)+sizeof(NodeID);
		Version networkVersion; // Version number of the network to which the message applies
		DragID newDragId; // ID of this drag operation
		VruiCoreProtocol::InputDeviceID deviceId; // ID of dragging device
		NodeID pickedNode; // The node picked by the dragger
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageBase) // Returns a message buffer for a drag start request message
			{
			return MessageBuffer::create(messageBase+DragStartRequest,size);
			}
		};
	
	struct DragRequestMsg
		{
		/* Elements: */
		public:
		static const size_t size=sizeof(Version)+sizeof(DragID);
		Version networkVersion; // Version number of the network to which the message applies
		DragID dragId; // ID of this drag operation
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageBase) // Returns a message buffer for a drag request message
			{
			return MessageBuffer::create(messageBase+DragRequest,size);
			}
		};
	
	struct DragStopRequestMsg
		{
		/* Elements: */
		public:
		static const size_t size=sizeof(Version)+sizeof(DragID);
		Version networkVersion; // Version number of the network to which the message applies
		DragID dragId; // ID of this drag operation
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageBase) // Returns a message buffer for a drag stop request message
			{
			return MessageBuffer::create(messageBase+DragStopRequest,size);
			}
		};
	
	struct SimulationUpdateMsg
		{
		/* Elements: */
		public:
		static const size_t size=sizeof(Version)+sizeof(Misc::UInt32); // Size up to and including number of particle positions in the message
		Version networkVersion; // Version number of the network to which the message applies
		Misc::UInt32 numParticles; // Number of particles whose positions are contained in this message
		// Point particlePositions[numParticles]; // Array of particle positions
		
		/* Methods: */
		static MessageBuffer* createMessage(unsigned int messageBase,size_t numParticles) // Returns a message buffer for a simulation update message containing the given number of particle positions
			{
			return MessageBuffer::create(messageBase+SimulationUpdate,size+numParticles*pointSize);
			}
		};
	
	/* Elements: */
	static const char* protocolName;
	static const unsigned int protocolVersion=4U<<16;
	};

}

}

#endif
