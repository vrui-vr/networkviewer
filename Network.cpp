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

#include "Network.h"

#include <string>
#include <deque>
#include <algorithm>
#include <stdexcept>
#include <Misc/StringHashFunctions.h>
#include <Misc/MessageLogger.h>
#include <Misc/ConvertColorComponent.h>
#include <IO/File.h>
#include <Math/Math.h>
#include <Math/Random.h>
#include <Math/Interval.h>

#include "JsonEntity.h"
#include "JsonBoolean.h"
#include "JsonNumber.h"
#include "JsonString.h"
#include "JsonMap.h"
#include "JsonFile.h"
#include "ParticleSystem.h"

/************************
Methods of class Network:
************************/

Network::Network(IO::File& networkFile)
	:selection(17)
	{
	/* Parse the root entity of the given json network file: */
	JsonMapPointer jsonRoot;
	{
	JsonFile jsonFile(networkFile);
	jsonRoot=jsonFile.parseEntity();
	}
	if(jsonRoot==0)
		throw std::runtime_error("Network::Network: JSON root entity is not a map");
	
	/* Find the node list: */
	jsonNodes=JsonListPointer(jsonRoot->getProperty("nodes"));
	if(jsonNodes==0)
		throw std::runtime_error("Network::Network: Nodes entity is not a list");
	
	/* Parse the node list: */
	Misc::HashTable<std::string,unsigned int> nodeIndices(17);
	unsigned int nodeIndex=0;
	nodes.reserve(jsonNodes->size());
	for(JsonList::List::iterator nIt=jsonNodes->getList().begin();nIt!=jsonNodes->getList().end();++nIt,++nodeIndex)
		{
		/* Parse a node from the node list item: */
		JsonMapPointer nodeMap(*nIt);
		if(nodeMap==0)
			throw std::runtime_error("Network::Network: Node entity is not a map");
		nodes.push_back(Node(nodeMap,nodeIndex));
		
		/* Associate the node's ID with its index in the list: */
		nodeIndices[nodes.back().getId()]=nodeIndex;
		}
	
	// DEBUGGING
	Misc::formattedLogNote("Network: Parsed %u nodes",(unsigned int)(nodes.size()));
	
	/* Initialize the node color mapping: */
	nodeColors.clear();
	nodeColors.reserve(nodes.size());
	for(NodeList::iterator nIt=nodes.begin();nIt!=nodes.end();++nIt)
		nodeColors.push_back(nIt->getColor());
	mapNodeColorsFromNode();
	
	/* Collect the names of all properties appearing in the network's nodes: */
	{
	Misc::HashTable<std::string,void> nodePropertyNameMap(17);
	for(JsonList::List::iterator nIt=jsonNodes->getList().begin();nIt!=jsonNodes->getList().end();++nIt)
		{
		/* Iterate through the node's properties: */
		const JsonMap::Map& nodeProperties=JsonMapPointer(*nIt)->getMap();
		for(JsonMap::Map::ConstIterator npIt=nodeProperties.begin();!npIt.isFinished();++npIt)
			{
			/* Collect the property's name: */
			nodePropertyNameMap.setEntry(Misc::HashTable<std::string,void>::Entry(npIt->getSource()));
			}
		}
	
	/* Put the collected property names into a list: */
	nodePropertyNames.reserve(nodePropertyNameMap.getNumEntries());
	for(Misc::HashTable<std::string,void>::Iterator npnIt=nodePropertyNameMap.begin();!npnIt.isFinished();++npnIt)
		nodePropertyNames.push_back(npnIt->getSource());
	}
	
	/* Sort the list of node property names: */
	std::sort(nodePropertyNames.begin(),nodePropertyNames.end());
	
	/* Find the link list: */
	jsonLinks=JsonListPointer(jsonRoot->getProperty("links"));
	if(jsonLinks==0)
		throw std::runtime_error("Network::Network: Links entity is not a list");
	
	/* Parse the link list: */
	links.reserve(jsonLinks->size());
	for(JsonList::List::iterator lIt=jsonLinks->getList().begin();lIt!=jsonLinks->getList().end();++lIt)
		{
		/* Parse a link from the link list item: */
		JsonMapPointer linkMap(*lIt);
		if(linkMap==0)
			throw std::runtime_error("Network::Network: Link entity is not a map");
		const std::string& sourceId=getString(linkMap->getProperty("source"));
		unsigned int sourceIndex=nodeIndices.getEntry(sourceId).getDest();
		const std::string& targetId=getString(linkMap->getProperty("target"));
		unsigned int targetIndex=nodeIndices.getEntry(targetId).getDest();
		Scalar value(1);
		if(linkMap->hasProperty("value"))
			value=Scalar(getNumber(linkMap->getProperty("value")));
		links.push_back(Link(&nodes[sourceIndex],sourceIndex,&nodes[targetIndex],targetIndex,value));
		
		/* Connect the two linked nodes: */
		nodes[sourceIndex].addLinkedNode(&nodes[targetIndex]);
		nodes[targetIndex].addLinkedNode(&nodes[sourceIndex]);
		}
	
	// DEBUGGING
	Misc::formattedLogNote("Network: Parsed %u links",(unsigned int)(links.size()));
	
	/* Create a color map for node distances: */
	static const GLColorMap::Color sdColors[6]=
		{
		GLColorMap::Color(1.0f,0.0f,0.0f),
		GLColorMap::Color(1.0f,1.0f,0.0f),
		GLColorMap::Color(0.0f,1.0f,0.0f),
		GLColorMap::Color(0.0f,1.0f,1.0f),
		GLColorMap::Color(0.0f,0.0f,1.0f),
		GLColorMap::Color(1.0f,0.0f,1.0f)
		};
	GLdouble sdKeys[6];
	for(int i=0;i<6;++i)
		sdKeys[i]=double(i);
	selectionDistanceMap.setColors(6,sdColors,sdKeys);
	}

Network::~Network(void)
	{
	}

void Network::mapNodeColorsFromNode(void)
	{
	/* Set all nodes' colors to their color properties' values: */
	ColorList::iterator ncIt=nodeColors.begin();
	for(NodeList::iterator nIt=nodes.begin();nIt!=nodes.end();++nIt,++ncIt)
		*ncIt=nIt->getColor();
	}

namespace {

/****************
Helper functions:
****************/

Node::Color randomColor(void)
	{
	typedef Misc::ColorComponentTraits<Node::Color::Scalar> NodeColorTraits;
	Node::Color result;
	for(int i=0;i<3;++i)
		result[i]=Node::Color::Scalar(Math::randUniformCC(NodeColorTraits::zero,NodeColorTraits::one));
	result[3]=NodeColorTraits::one;
	return result;
	}

Node::Color convertColor(const GLColorMap::Color& glColor)
	{
	Node::Color result;
	for(int i=0;i<4;++i)
		result[i]=Misc::convertColorComponent<Node::Color::Scalar,GLColorMap::Color::Scalar>(glColor[i]);
	return result;
	}

}

void Network::mapNodeColorsFromNodeProperty(const std::string& propertyName,GLColorMap& numericalPropertyValueMap)
	{
	/* Collect the set of values of the given property from all nodes: */
	Misc::HashTable<bool,Node::Color> booleanValueColors(17);
	Misc::HashTable<std::string,Node::Color> stringValueColors(17);
	Math::Interval<double> numberValueRange=Math::Interval<double>::empty;
	for(JsonList::List::iterator nIt=jsonNodes->getList().begin();nIt!=jsonNodes->getList().end();++nIt)
		{
		/* Get the map of node properties and check if the node has the requested property: */
		JsonMapPointer nodeMap(*nIt);
		if(nodeMap->hasProperty(propertyName))
			{
			/* Get the property's value and handle it based on its type: */
			JsonPointer value=nodeMap->getProperty(propertyName);
			switch(value->getType())
				{
				case JsonEntity::BOOLEAN:
					{
					bool bValue=getBoolean(value);
					if(!booleanValueColors.isEntry(bValue))
						booleanValueColors[bValue]=randomColor();
					break;
					}
				
				case JsonEntity::NUMBER:
					numberValueRange.addValue(getNumber(value));
					break;
				
				case JsonEntity::STRING:
					{
					const std::string& sValue=getString(value);
					if(!stringValueColors.isEntry(sValue))
						stringValueColors[sValue]=randomColor();
					break;
					}
				
				default:
					;
				}
			}
		}
	
	/* Assign colors to all nodes: */
	numericalPropertyValueMap.setScalarRange(numberValueRange.getMin(),numberValueRange.getMax());
	ColorList::iterator ncIt=nodeColors.begin();
	for(JsonList::List::iterator nIt=jsonNodes->getList().begin();nIt!=jsonNodes->getList().end();++nIt,++ncIt)
		{
		/* Get the map of node properties and check if the node has the requested property: */
		JsonMapPointer nodeMap(*nIt);
		if(nodeMap->hasProperty(propertyName))
			{
			/* Get the property's value and handle it based on its type: */
			JsonPointer value=nodeMap->getProperty(propertyName);
			switch(value->getType())
				{
				case JsonEntity::BOOLEAN:
					*ncIt=booleanValueColors[getBoolean(value)].getDest();
					break;
				
				case JsonEntity::NUMBER:
					*ncIt=convertColor(numericalPropertyValueMap(getNumber(value)));
					break;
				
				case JsonEntity::STRING:
					*ncIt=stringValueColors[getString(value)].getDest();
					break;
				
				default:
					/* Color nodes with unsupported property types grey: */
					*ncIt=Node::Color(128U,128U,128U);
				}
			}
		else
			{
			/* Color nodes without the requested property grey: */
			*ncIt=Node::Color(128U,128U,128U);
			}
		}
	}

void Network::mapNodeColorsFromSelectionDistance(void)
	{
	/* Calculate the distance of all nodes from the current selection: */
	unsigned int numNodes=(unsigned int)(nodes.size());
	std::vector<unsigned int> nodeDistances;
	nodeDistances.reserve(numNodes);
	for(NodeList::iterator nIt=nodes.begin();nIt!=nodes.end();++nIt)
		nodeDistances.push_back(numNodes);
	
	/* Initialize the distance of all selected nodes to zero and add them into the breadth-first queue: */
	std::deque<unsigned int> queue;
	for(Selection::Iterator sIt=selection.begin();!sIt.isFinished();++sIt)
		{
		nodeDistances[sIt->getSource()]=0;
		queue.push_back(sIt->getSource());
		}
	
	/* Traverse connected nodes in breadth-first order: */
	unsigned int maxNodeDistance=0;
	while(!queue.empty())
		{
		/* Grab the next node in the breadth-first queue: */
		unsigned int nodeIndex=queue.front();
		queue.pop_front();
		Node& node=nodes[nodeIndex];
		
		/* Assign distances to all nodes connected to the next node in the queue: */
		unsigned int nodeDistance=nodeDistances[nodeIndex];
		for(std::vector<Node*>::const_iterator lnIt=node.getLinkedNodes().begin();lnIt!=node.getLinkedNodes().end();++lnIt)
			{
			/* Get the linked node's index: */
			unsigned int linkedNodeIndex=*lnIt-&nodes.front();
			
			/* Check if the linked node has not yet been visited: */
			if(nodeDistances[linkedNodeIndex]==numNodes)
				{
				/* Assign a distance to the linked node and add it into the breadth-first queue: */
				nodeDistances[linkedNodeIndex]=nodeDistance+1;
				if(maxNodeDistance<nodeDistance+1)
					maxNodeDistance=nodeDistance+1;
				queue.push_back(linkedNodeIndex);
				}
			else
				{
				/* Check that the linked node does not have a larger distance than as a neighbor of the next node: */
				if(nodeDistances[linkedNodeIndex]>nodeDistance+1)
					throw std::runtime_error("NetworkViewer::mapNodeColorsFromSelectionDistance: Took the long way around in network traversal");
				}
			}
		}
	
	/* Assign colors to nodes based on their computed distances: */
	selectionDistanceMap.setScalarRange(0.0,double(maxNodeDistance));
	std::vector<unsigned int>::iterator ndIt=nodeDistances.begin();
	for(ColorList::iterator ncIt=nodeColors.begin();ncIt!=nodeColors.end();++ncIt,++ndIt)
		{
		if(*ndIt<numNodes)
			{
			/* Assign a color based on the distance color map: */
			*ncIt=convertColor(selectionDistanceMap(double(*ndIt)));
			}
		else
			{
			/* Color unconnected nodes grey: */
			*ncIt=Node::Color(128U,128U,128U);
			}
		}
	}

void Network::createParticles(ParticleSystem& particles,Scalar linkStrength)
	{
	/* Calculate an appropriate domain size: */
	Scalar domainSize=Math::pow(Scalar(nodes.size()),Scalar(1.0/3.0));
	
	/* Create one particle for each node: */
	for(NodeList::iterator nIt=nodes.begin();nIt!=nodes.end();++nIt)
		nIt->createParticle(particles,domainSize);
	
	/* Create a distance constraint for each pair of linked nodes: */
	for(LinkList::iterator lIt=links.begin();lIt!=links.end();++lIt)
		{
		/* Add a distance constraint for the two linked nodes' particles to the particle system: */
		particles.addDistConstraint(lIt->getNode(0)->getParticleIndex(),lIt->getNode(1)->getParticleIndex(),1,lIt->getValue()*linkStrength);
		}
	}

void Network::clearSelection(void)
	{
	/* Clear the selection set: */
	selection.clear();
	
	/* Color nodes from their color property: */
	mapNodeColorsFromNode();
	}

void Network::setSelection(unsigned int nodeIndex)
	{
	/* Clear the selection set: */
	selection.clear();
	
	/* Select the given node: */
	selection.setEntry(Selection::Entry(nodeIndex));
	
	/* Color nodes by distance to the selection: */
	mapNodeColorsFromSelectionDistance();
	}

void Network::selectNode(unsigned int nodeIndex)
	{
	/* Select the given node: */
	selection.setEntry(Selection::Entry(nodeIndex));
	
	/* Color nodes by distance to the selection: */
	mapNodeColorsFromSelectionDistance();
	}

void Network::deselectNode(unsigned int nodeIndex)
	{
	/* Deselect the given node: */
	selection.removeEntry(nodeIndex);
	
	if(selection.getNumEntries()!=0)
		{
		/* Color nodes by distance to the selection: */
		mapNodeColorsFromSelectionDistance();
		}
	else
		{
		/* Color nodes from their color property: */
		mapNodeColorsFromNode();
		}
	}

void Network::growSelection(void)
	{
	/* Extract all currently selected nodes: */
	std::vector<unsigned int> selectedNodes;
	selectedNodes.reserve(selection.getNumEntries());
	for(Selection::Iterator sIt=selection.begin();!sIt.isFinished();++sIt)
		selectedNodes.push_back(sIt->getSource());
	
	/* Iterate through the selected nodes: */
	for(std::vector<unsigned int>::iterator sIt=selectedNodes.begin();sIt!=selectedNodes.end();++sIt)
		{
		/* Select all nodes linked to the current selected node: */
		Node& node=nodes[*sIt];
		for(std::vector<Node*>::const_iterator lnIt=node.getLinkedNodes().begin();lnIt!=node.getLinkedNodes().end();++lnIt)
			{
			/* Get the linked node's index: */
			unsigned int linkedNodeIndex=*lnIt-&nodes.front();
			
			/* Select the linked node: */
			selection.setEntry(Selection::Entry(linkedNodeIndex));
			}
		}
	
	/* Color nodes by distance to the selection: */
	mapNodeColorsFromSelectionDistance();
	}

void Network::shrinkSelection(void)
	{
	/* Iterate through the selected nodes: */
	std::vector<unsigned int> deselectNodes; // List of nodes to be deselected: */
	for(Selection::Iterator sIt=selection.begin();!sIt.isFinished();++sIt)
		{
		/* Check if the current selected node is linked to any non-selected nodes: */
		Node& node=nodes[sIt->getSource()];
		bool allLinkedNodesSelected=true;
		for(std::vector<Node*>::const_iterator lnIt=node.getLinkedNodes().begin();allLinkedNodesSelected&&lnIt!=node.getLinkedNodes().end();++lnIt)
			{
			/* Get the linked node's index: */
			unsigned int linkedNodeIndex=*lnIt-&nodes.front();
			
			/* Check if the linked node is selected: */
			allLinkedNodesSelected=selection.isEntry(linkedNodeIndex);
			}
		
		/* Mark the node for deselection if not all its linked nodes are selected: */
		if(!allLinkedNodesSelected)
			deselectNodes.push_back(sIt->getSource());
		}
	
	/* Deselect all marked nodes: */
	for(std::vector<unsigned int>::iterator dnIt=deselectNodes.begin();dnIt!=deselectNodes.end();++dnIt)
		{
		/* Deselect the linked node: */
		selection.removeEntry(*dnIt);
		}
	
	/* Color nodes by distance to the selection: */
	mapNodeColorsFromSelectionDistance();
	}
