/***********************************************************************
Network - Class representing a network of nodes and links parsed from a
json file.
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

#ifndef NETWORK_INCLUDED
#define NETWORK_INCLUDED

#include <string>
#include <vector>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <GL/gl.h>
#include <GL/GLColorMap.h>

#include "ParticleTypes.h"
#include "JsonList.h"
#include "JsonMap.h"
#include "Node.h"
#include "Link.h"

/* Forward declarations: */
namespace IO {
class File;
}
class GLColorMap;
class ParticleSystem;

class Network
	{
	/* Embedded classes: */
	public:
	typedef std::vector<Node> NodeList;
	typedef std::vector<Node::Color> ColorList;
	typedef std::vector<Link> LinkList;
	typedef std::vector<std::string> StringList;
	typedef Misc::HashTable<unsigned int,void> Selection; // Type of hash tables to mark selected nodes
	
	/* Elements: */
	private:
	JsonListPointer jsonNodes; // Pointer to the list of node entities parsed from the network json file
	JsonListPointer jsonLinks; // Pointer to the list of link entities parsed from the network json file
	NodeList nodes; // List of nodes
	ColorList nodeColors; // Current color values assigned to each node
	StringList nodePropertyNames; // List of names of all node properties defined in the network json file
	LinkList links; // List of links
	Selection selection; // Set of currenty selected nodes
	GLColorMap selectionDistanceMap; // Color map mapping node distances from the selection to colors
	
	/* Constructors and destructors: */
	public:
	Network(IO::File& networkFile); // Parses a network from a json file
	private:
	Network(const Network& source); // Prohibit copy constructor
	Network& operator=(const Network& source); // Prohibit assignment operator
	public:
	~Network(void); // Destroys the network
	
	/* Methods: */
	void mapNodeColorsFromNode(void); // Retrieves node colors from the nodes' color properties
	void mapNodeColorsFromNodeProperty(const std::string& propertyName,GLColorMap& numericalPropertyValueMap); // Colors nodes by their values of the given property
	void mapNodeColorsFromSelectionDistance(void); // Colors nodes by their distance from the current selection
	void createParticles(ParticleSystem& particles,Scalar linkStrength); // Creates particles representing the network's nodes and distance constraints representing its links
	const NodeList& getNodes(void) const // Returns the array of nodes
		{
		return nodes;
		}
	const ColorList& getNodeColors(void) const // Returns the array of node colors
		{
		return nodeColors;
		}
	const StringList& getNodePropertyNames(void) const // Returns the list of all node property names found in the network json file
		{
		return nodePropertyNames;
		}
	const LinkList& getLinks(void) const // Returns the array of links
		{
		return links;
		}
	const JsonMap::Map& getNodeProperties(unsigned int nodeIndex) const // Returns the properties of the node of the given index
		{
		return JsonMapPointer(jsonNodes->getItem(nodeIndex))->getMap();
		}
	void clearSelection(void); // De-selects all selected nodes
	void setSelection(unsigned int nodeIndex); // Selects only the node of the given index
	void selectNode(unsigned int nodeIndex); // Adds the node of the given index to the selection
	void deselectNode(unsigned int nodeIndex); // Removes the node of the given index from the selection
	void growSelection(void); // Selects all nodes that are directly linked to already selected nodes
	void shrinkSelection(void); // De-selects all nodes that are linked to nodes that are not currently selected
	const Selection& getSelection(void) const // Returns the set of currently selected nodes
		{
		return selection;
		}
	size_t getSelectionSize(void) const // Returns the number of currently selected nodes
		{
		return selection.getNumEntries();
		}
	bool isSelected(unsigned int nodeIndex) const // Returns true if the node of the given index is currently selected
		{
		return selection.isEntry(nodeIndex);
		}
	
	};

#endif
