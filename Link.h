/***********************************************************************
Link - Class for links between network nodes.
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

#ifndef LINK_INCLUDED
#define LINK_INCLUDED

#include "Node.h"

class Link
	{
	/* Elements: */
	private:
	Node* nodes[2]; // Pointer to the two linked nodes
	unsigned int nodeIndices[2]; // Indices of the two linked nodes in a node state array
	Scalar value; // Strength of the link between the linked nodes
	
	/* Constructors and destructors: */
	public:
	Link(Node* sNode0,unsigned int sNodeIndex0,Node* sNode1,unsigned int sNodeIndex1,Scalar sValue)
		:value(sValue)
		{
		nodes[0]=sNode0;
		nodes[1]=sNode1;
		nodeIndices[0]=sNodeIndex0;
		nodeIndices[1]=sNodeIndex1;
		}
	
	/* Methods: */
	const Node* getNode(int index) const // Returns one of the linked nodes
		{
		return nodes[index];
		}
	unsigned int getNodeIndex(int index) const // Returns the index of one of the linked nodes
		{
		return nodeIndices[index];
		}
	Scalar getValue(void) const // Returns the link value
		{
		return value;
		}
	};

#endif
