/***********************************************************************
Node - Class for network nodes.
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

#ifndef NODE_INCLUDED
#define NODE_INCLUDED

#include <string>
#include <vector>
#include <Misc/SizedTypes.h>
#include <Misc/RGBA.h>

#include "ParticleTypes.h"
#include "JsonMap.h"

/* Forward declarations: */
class ParticleSystem;

class Node
	{
	/* Embedded classes: */
	public:
	typedef Misc::RGBA<Misc::UInt8> Color; // Type for colors
	
	/* Elements: */
	private:
	std::string id; // Node's ID
	Scalar size; // Node's size
	Color color; // Node's color
	Index particleIndex; // Index of the particle representing this node in the particle system
	std::vector<Node*> linkedNodes; // List of pointers to other nodes connected to this node via links
	
	/* Constructors and destructors: */
	public:
	Node(JsonMapPointer jsonMap,Index sParticleIndex =-1); // Creates a node from a json entity with name/value pairs
	
	/* Methods: */
	void createParticle(ParticleSystem& particles,Scalar domainSize); // Adds a particle representing the node to the given particle system
	const std::string& getId(void) const // Returns the node's ID
		{
		return id;
		}
	Scalar getSize(void) const // Returns the node's size
		{
		return size;
		}
	const Color& getColor(void) const // Returns the node's color
		{
		return color;
		}
	Index getParticleIndex(void) const // Returns the index of the particle representing the node
		{
		return particleIndex;
		}
	void addLinkedNode(Node* newLinkedNode) // Connects this node to the given other node
		{
		linkedNodes.push_back(newLinkedNode);
		}
	const std::vector<Node*>& getLinkedNodes(void) const // Returns the list of other nodes linked to this node
		{
		return linkedNodes;
		}
	};

#endif
